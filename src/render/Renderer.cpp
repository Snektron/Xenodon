#include "render/Renderer.h"
#include <array>
#include <algorithm>
#include <iterator>
#include <utility>
#include "utility/rect_union.h"
#include "core/Logger.h"
#include "graphics/shader/Shader.h"
#include "graphics/utility.h"
#include "math/Vec.h"

namespace {
    // The local group size all RenderAlgorithm shaders should have
    constexpr const Vec2<uint32_t> LOCAL_SIZE{8, 8};

    // Standard bindings all RenderAlgorithm shaders should have
    const auto STANDARD_BINDINGS = std::array {
        vk::DescriptorSetLayoutBinding(
            0, // layout(binding = 0) uniform OutputRegionBuffer
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eCompute
        ),
        vk::DescriptorSetLayoutBinding(
            1, // layout(binding = 1) restrict writeonly uniform image2D render_target
            vk::DescriptorType::eStorageImage,
            1,
            vk::ShaderStageFlagBits::eCompute
        )
    };
}

Renderer::Renderer(std::shared_ptr<RenderContext> ctx, size_t device_index):
    ctx(ctx),
    device_index(device_index),
    rendev(&this->ctx->display->render_device(this->device_index)),
    stats_collector(this->ctx->display, this->device_index) {

    this->create_resources();
    this->upload_uniform_buffers();

    this->create_descriptor_sets();
    this->update_descriptor_sets();
    this->create_command_buffers();
}

void Renderer::recreate(size_t output) {
    auto& orsc = this->output_resources[output];
    orsc.region = orsc.output->region();

    const uint32_t images = this->ctx->display->output(this->device_index, output)->num_swap_images();
    if (static_cast<size_t>(images) != this->output_resources[output].command_buffers.size()) {
        this->create_descriptor_sets();
        this->create_command_buffers();
    }

    for (size_t outputidx = 0; outputidx < this->output_resources.size(); ++outputidx) {
        auto& orsc = this->output_resources[outputidx];
        orsc.region = orsc.output->region();
    }

    this->update_descriptor_sets();
    this->resize();
}

void Renderer::resize() {
    this->upload_uniform_buffers();
}

void Renderer::render(const Camera& cam) {
    const auto begin_info = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

    const auto push_constants = PushConstantBuffer {
        .camera = {
            Vec4F(cam.forward, 0),
            Vec4F(cam.up, 0),
            Vec4F(cam.translation / this->ctx->shader_params.voxel_ratio.xyz, 0) // pre-divide
        },
    };

    for (size_t outputidx = 0; outputidx < this->output_resources.size(); ++outputidx) {
        auto& orsc = this->output_resources[outputidx];

        uint32_t index = orsc.output->current_swap_index();
        const auto swap_image = orsc.output->swap_image(index);
        const auto attachment = orsc.output->color_attachment_descr();
        auto& cmd_buf = orsc.command_buffers[index].get();

        auto group_size = (Vec2<uint32_t>{orsc.region.extent.width, orsc.region.extent.height} - 1u) / LOCAL_SIZE + 1u;

        cmd_buf.begin(&begin_info);

        image_transition(
            cmd_buf,
            swap_image.image,
            {attachment.initialLayout, vk::PipelineStageFlagBits::eTopOfPipe},
            {vk::ImageLayout::eGeneral, vk::PipelineStageFlagBits::eComputeShader}
        );

        cmd_buf.bindPipeline(vk::PipelineBindPoint::eCompute, this->pipeline.get());
        cmd_buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, this->pipeline_layout.get(), 0, orsc.descriptor_sets[index], nullptr);
        cmd_buf.pushConstants(this->pipeline_layout.get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(PushConstantBuffer), static_cast<const void*>(&push_constants));
        this->stats_collector.pre_dispatch(outputidx, cmd_buf);
        cmd_buf.dispatch(group_size.x, group_size.y, 1);
        this->stats_collector.post_dispatch(outputidx, cmd_buf);

        image_transition(
            cmd_buf,
            swap_image.image,
            {vk::ImageLayout::eGeneral, vk::PipelineStageFlagBits::eComputeShader},
            {attachment.finalLayout, vk::PipelineStageFlagBits::eBottomOfPipe}
        );

        cmd_buf.end();

        swap_image.submit(this->rendev->compute_queue, cmd_buf, vk::PipelineStageFlagBits::eBottomOfPipe);
    }
}

void Renderer::collect_stats() {
    this->stats_collector.collect();
}

RenderStats Renderer::stats() const {
    return this->stats_collector.stats();
}

void Renderer::create_resources() {
    const auto& device = this->rendev->device;
    const uint32_t outputs = static_cast<uint32_t>(this->rendev->outputs);

    this->resources = this->ctx->algorithm->upload_resources(*this->rendev);

    this->descriptor_set_layout = device->createDescriptorSetLayoutUnique({
        {},
        static_cast<uint32_t>(this->ctx->bindings.size()),
        this->ctx->bindings.data()
    });

    const auto shader = Shader(device, vk::ShaderStageFlagBits::eCompute, this->ctx->algorithm->shader());

    const auto push_constant_range = vk::PushConstantRange(
        vk::ShaderStageFlagBits::eCompute,
        0,
        sizeof(PushConstantBuffer)
    );

    this->pipeline_layout = device->createPipelineLayoutUnique({
        {},
        1,
        &this->descriptor_set_layout.get(),
        1,
        &push_constant_range
    });

    this->uniform_buffer = std::make_unique<Buffer<UniformBuffer>>(
        device,
        outputs,
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    this->pipeline = device->createComputePipelineUnique(vk::PipelineCache(), {
        {},
        shader.info(),
        this->pipeline_layout.get()
    });

    this->output_resources.reserve(outputs);
    for (size_t j = 0; j < outputs; ++j) {
        Output* output = this->ctx->display->output(this->device_index, j);
        this->output_resources.emplace_back(OutputResources{
            output,
            output->region(),
            Span<vk::DescriptorSet>(nullptr),
            std::vector<vk::UniqueCommandBuffer>()
        });
    }
}

void Renderer::create_descriptor_sets() {
    const auto& device = this->rendev->device;

    uint32_t total_images = 0;
    for (const auto& orsc : this->output_resources) {
        total_images += orsc.output->num_swap_images();
    }

    this->descriptor_pool = create_descriptor_pool(device, total_images);

    const auto descr_set_layouts = std::vector<vk::DescriptorSetLayout>(total_images, this->descriptor_set_layout.get());
    const auto descr_alloc_info = vk::DescriptorSetAllocateInfo(
        this->descriptor_pool.get(),
        total_images,
        descr_set_layouts.data()
    );

    this->descriptor_sets = device->allocateDescriptorSets(descr_alloc_info);

    size_t descriptor_set_base = 0;
    for (size_t outputidx = 0; outputidx < this->output_resources.size(); ++outputidx) {
        auto& orsc = this->output_resources[outputidx];
        const uint32_t images = orsc.output->num_swap_images();

        orsc.descriptor_sets = Span(images, &this->descriptor_sets[descriptor_set_base]);
        descriptor_set_base += images;
    }
}

void Renderer::create_command_buffers() {
    for (size_t outputidx = 0; outputidx < this->output_resources.size(); ++outputidx) {
        auto& orsc = this->output_resources[outputidx];
        const uint32_t images = orsc.output->num_swap_images();

        orsc.command_buffers = this->rendev->compute_command_pool.allocate_command_buffers(images);
    }
}

void Renderer::update_descriptor_sets() {
    for (size_t outputidx = 0; outputidx < this->output_resources.size(); ++outputidx) {
        auto& orsc = this->output_resources[outputidx];
        const auto uniform_buffer_info = this->uniform_buffer->descriptor_info(outputidx, 1);
        const uint32_t images = orsc.output->num_swap_images();

        for (uint32_t image = 0; image < images; ++image) {
            auto swap_image = orsc.output->swap_image(image);
            auto& set = orsc.descriptor_sets[image];

            const auto render_target_info = vk::DescriptorImageInfo(
                vk::Sampler(),
                swap_image.view,
                vk::ImageLayout::eGeneral
            );

            const auto descriptor_writes = std::array{
                write_set(set, STANDARD_BINDINGS[0], uniform_buffer_info),
                write_set(set, STANDARD_BINDINGS[1], render_target_info)
            };

            this->rendev->device->updateDescriptorSets(descriptor_writes, nullptr);

            this->resources->update_descriptors(set);
        }
    }
}

void Renderer::upload_uniform_buffers() {
    const auto& device = this->rendev->device;
    const uint32_t outputs = static_cast<uint32_t>(this->rendev->outputs);

    auto staging_buffer = Buffer<UniformBuffer>(
        device,
        outputs,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    UniformBuffer* uniforms = staging_buffer.map(0, outputs);

    for (size_t outputidx = 0; outputidx < this->output_resources.size(); ++outputidx) {
        auto& orsc = this->output_resources[outputidx];

        uniforms[outputidx].output_region = orsc.region;
        uniforms[outputidx].display_region = this->ctx->display_region;
        uniforms[outputidx].params = this->ctx->shader_params;
    }

    staging_buffer.unmap();

    const auto copy_info = vk::BufferCopy{
        0,
        0,
        sizeof(UniformBuffer) * outputs
    };

    this->rendev->compute_command_pool.one_time_submit([&](vk::CommandBuffer cmd_buf) {
        cmd_buf.copyBuffer(staging_buffer.get(), this->uniform_buffer->get(), copy_info);
    });
}

vk::UniqueDescriptorPool Renderer::create_descriptor_pool(const Device& device, uint32_t sets) {
    auto pool_sizes = std::vector<vk::DescriptorPoolSize>();

    auto find_pool_size = [&pool_sizes](vk::DescriptorType type) -> vk::DescriptorPoolSize* {
        for (auto& psz : pool_sizes) {
            if (psz.type == type) {
                return &psz;
            }
        }

        return nullptr;
    };

    for (const auto& binding : this->ctx->bindings) {
        if (auto* pool_size = find_pool_size(binding.descriptorType)) {
            pool_size->descriptorCount += sets;
        } else {
            pool_sizes.emplace_back(binding.descriptorType, sets);
        }
    }

    const auto descr_pool_create_info = vk::DescriptorPoolCreateInfo(
        {},
        sets,
        static_cast<uint32_t>(pool_sizes.size()),
        pool_sizes.data()
    );

    return device->createDescriptorPoolUnique(descr_pool_create_info);
}
