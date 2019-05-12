#include "render/ComputeSvoRaytracer.h"
#include <string_view>
#include "core/Logger.h"
#include "graphics/shader/Shader.h"
#include "utility/rect_union.h"
#include "math/Vec.h"
#include "resources.h"

namespace {
    constexpr const std::string_view SHADER_RESOURCE = "resources/svo.comp";
    constexpr const Vec2<uint32_t> LOCAL_SIZE{8, 8}; // keep in sync with above shader

    const auto LAYOUT_BINDINGS = std::array {
        vk::DescriptorSetLayoutBinding(
            0, // layout(binding = 0) uniform OutputRegionBuffer
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eCompute
        ),
        vk::DescriptorSetLayoutBinding(
            1, // layout(binding = 1) readonly buffer Octree
            vk::DescriptorType::eStorageBuffer,
            1,
            vk::ShaderStageFlagBits::eCompute
        ),
        vk::DescriptorSetLayoutBinding(
            2, // layout(binding = 2) restrict writeonly uniform image2D render_target
            vk::DescriptorType::eStorageImage,
            1,
            vk::ShaderStageFlagBits::eCompute
        )
    };

    vk::WriteDescriptorSet write_set(const vk::DescriptorSet& set, const vk::DescriptorSetLayoutBinding& binding, const vk::DescriptorImageInfo& image_info) {
        return vk::WriteDescriptorSet(
            set,
            binding.binding,
            0,
            1,
            binding.descriptorType,
            &image_info,
            nullptr,
            nullptr
        );
    }

    vk::WriteDescriptorSet write_set(const vk::DescriptorSet& set, const vk::DescriptorSetLayoutBinding& binding, const vk::DescriptorBufferInfo& buffer_info) {
        return vk::WriteDescriptorSet(
            set,
            binding.binding,
            0,
            1,
            binding.descriptorType,
            nullptr,
            &buffer_info,
            nullptr
        );
    }

    struct ImageState {
        vk::ImageLayout layout;
        vk::PipelineStageFlagBits stage;
    };

    void image_transition(vk::CommandBuffer cmd_buf, vk::Image image, ImageState src, ImageState dst) {
        auto barrier = vk::ImageMemoryBarrier(
            vk::AccessFlags(),
            vk::AccessFlags(),
            src.layout,
            dst.layout,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            image,
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
        );

        cmd_buf.pipelineBarrier(
            src.stage,
            dst.stage,
            vk::DependencyFlags(),
            nullptr,
            nullptr,
            barrier
        );
    }

    vk::UniqueDescriptorPool create_descriptor_pool(const RenderDevice& rendev, uint32_t sets) {
        constexpr const size_t num_bindings = std::tuple_size_v<decltype(LAYOUT_BINDINGS)>;
        auto pool_sizes = std::array<vk::DescriptorPoolSize, num_bindings>{};

        for (size_t i = 0; i < num_bindings; ++i) {
            pool_sizes[i] = vk::DescriptorPoolSize(LAYOUT_BINDINGS[i].descriptorType, sets);
        }

        const auto descr_pool_create_info = vk::DescriptorPoolCreateInfo(
            {},
            sets,
            static_cast<uint32_t>(pool_sizes.size()),
            pool_sizes.data()
        );

        return rendev.device->createDescriptorPoolUnique(descr_pool_create_info);
    }
}

ComputeSvoRaytracer::ComputeSvoRaytracer(Display* display, const Octree& model):
    display(display),
    model(model),
    start(std::chrono::system_clock::now()) {
    this->calculate_display_rect();
    this->create_resources();
    this->upload_uniform_buffers();
    this->upload_tree_buffers();

    this->create_descriptor_sets();
    this->update_descriptor_sets();
    this->create_command_buffers();
}

void ComputeSvoRaytracer::recreate(size_t device, size_t output) {
    const uint32_t images = this->display->output(device, output)->num_swap_images();

    for (size_t devidx = 0; devidx < this->device_resources.size(); ++devidx) {
        auto& drsc = this->device_resources[devidx];

        for (size_t outputidx = 0; outputidx < drsc.output_resources.size(); ++outputidx) {
            auto& orsc = drsc.output_resources[outputidx];
            orsc.region = orsc.output->region();
        }
    }

    this->calculate_display_rect();

    if (static_cast<size_t>(images) != this->device_resources[device].output_resources[output].command_buffers.size()) {
        LOGGER.log("Recreating device {}, output {} descriptor sets and command buffers", device, output);
        this->create_descriptor_sets();
        this->create_command_buffers();
    }

    this->update_descriptor_sets();
    this->upload_uniform_buffers();
}

void ComputeSvoRaytracer::render() {
    const auto begin_info = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
    const auto now = std::chrono::system_clock::now();
    const float time = std::chrono::duration<float>(now - this->start).count();

    for (size_t devidx = 0; devidx < this->device_resources.size(); ++devidx) {
        auto& drsc = this->device_resources[devidx];

        for (size_t outputidx = 0; outputidx < drsc.output_resources.size(); ++outputidx) {
            auto& orsc = drsc.output_resources[outputidx];

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

            cmd_buf.bindPipeline(vk::PipelineBindPoint::eCompute, orsc.pipeline.get());
            cmd_buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, drsc.pipeline_layout.get(), 0, orsc.descriptor_sets[index], nullptr);
            cmd_buf.pushConstants(drsc.pipeline_layout.get(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), static_cast<const void*>(&time));
            cmd_buf.dispatch(group_size.x, group_size.y, 1);

            image_transition(
                cmd_buf,
                swap_image.image,
                {vk::ImageLayout::eGeneral, vk::PipelineStageFlagBits::eComputeShader},
                {attachment.finalLayout, vk::PipelineStageFlagBits::eBottomOfPipe}
            );

            cmd_buf.end();

            swap_image.submit(drsc.rendev->compute_queue, cmd_buf, vk::PipelineStageFlagBits::eBottomOfPipe);
        }
    }

    this->display->swap_buffers();
}

void ComputeSvoRaytracer::create_resources() {
    const size_t num_devices = this->display->num_render_devices();
    this->device_resources.clear();
    this->device_resources.reserve(num_devices);

    for (size_t i = 0; i < num_devices; ++i) {
        const auto& rendev = this->display->render_device(i);
        const auto& device = rendev.device;
        const uint32_t outputs = static_cast<uint32_t>(rendev.outputs);

        auto descr_set_layout = device->createDescriptorSetLayoutUnique({
            {},
            static_cast<uint32_t>(LAYOUT_BINDINGS.size()),
            LAYOUT_BINDINGS.data()
        });

        // Create the pipeline resources
        const auto shader = Shader(device, vk::ShaderStageFlagBits::eCompute, resources::open(SHADER_RESOURCE));

        const auto push_constant_range = vk::PushConstantRange(
            vk::ShaderStageFlagBits::eCompute,
            0,
            sizeof(float)
        );

        auto pipeline_layout = device->createPipelineLayoutUnique({
            {},
            1,
            &descr_set_layout.get(),
            1,
            &push_constant_range
        });

        // Create device buffers
        auto uniform_buffer = Buffer<UniformBuffer>(
            device,
            outputs,
            vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        auto tree_buffer = Buffer<Octree::Node>(
            device,
            this->model.data().size(),
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        // Create output pipelines
        auto output_resources = std::vector<OutputResources>();
        output_resources.reserve(outputs);
        for (size_t j = 0; j < outputs; ++j) {
            Output* output = this->display->output(i, j);
            auto pipeline = device->createComputePipelineUnique(vk::PipelineCache(), {
                {},
                shader.info(),
                pipeline_layout.get()
            });

            output_resources.emplace_back(OutputResources{
                output,
                output->region(),
                std::move(pipeline),
                Span<vk::DescriptorSet>(nullptr),
                std::vector<vk::UniqueCommandBuffer>()
            });
        }

        this->device_resources.emplace_back(DeviceResources{
            &rendev,
            std::move(descr_set_layout),
            vk::UniqueDescriptorPool(),
            std::vector<vk::DescriptorSet>(),

            std::move(pipeline_layout),

            std::move(uniform_buffer),
            std::move(tree_buffer),

            std::move(output_resources)
        });
    }
}

void ComputeSvoRaytracer::create_descriptor_sets() {
    for (size_t devidx = 0; devidx < this->device_resources.size(); ++devidx) {
        auto& drsc = this->device_resources[devidx];
        const auto& rendev = *drsc.rendev;
        const auto& device = rendev.device;

        uint32_t total_images = 0;
        for (const auto& orsc : drsc.output_resources) {
            total_images += orsc.output->num_swap_images();
        }

        drsc.descriptor_pool = create_descriptor_pool(rendev, total_images);

        const auto descr_set_layouts = std::vector<vk::DescriptorSetLayout>(total_images, drsc.descriptor_set_layout.get());
        const auto descr_alloc_info = vk::DescriptorSetAllocateInfo(
            drsc.descriptor_pool.get(),
            total_images,
            descr_set_layouts.data()
        );

        drsc.descriptor_sets = device->allocateDescriptorSets(descr_alloc_info);

        size_t descriptor_set_base = 0;
        for (size_t outputidx = 0; outputidx < drsc.output_resources.size(); ++outputidx) {
            auto& orsc = drsc.output_resources[outputidx];
            const uint32_t images = orsc.output->num_swap_images();

            orsc.descriptor_sets = Span(images, &drsc.descriptor_sets[descriptor_set_base]);
            descriptor_set_base += images;
        }
    }
}

void ComputeSvoRaytracer::create_command_buffers() {
    for (size_t devidx = 0; devidx < this->device_resources.size(); ++devidx) {
        auto& drsc = this->device_resources[devidx];

        for (size_t outputidx = 0; outputidx < drsc.output_resources.size(); ++outputidx) {
            auto& orsc = drsc.output_resources[outputidx];
            const uint32_t images = orsc.output->num_swap_images();

            orsc.command_buffers = drsc.rendev->compute_command_pool.allocate_command_buffers(images);
        }
    }
}

void ComputeSvoRaytracer::update_descriptor_sets() {
    for (size_t devidx = 0; devidx < this->device_resources.size(); ++devidx) {
        auto& drsc = this->device_resources[devidx];

        for (size_t outputidx = 0; outputidx < drsc.output_resources.size(); ++outputidx) {
            auto& orsc = drsc.output_resources[outputidx];
            const auto tree_buffer_info = drsc.tree_buffer.descriptor_info(0, this->model.data().size());
            const auto uniform_buffer_info = drsc.uniform_buffer.descriptor_info(outputidx, 1);
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
                    write_set(set, LAYOUT_BINDINGS[0], uniform_buffer_info),
                    write_set(set, LAYOUT_BINDINGS[1], tree_buffer_info),
                    write_set(set, LAYOUT_BINDINGS[2], render_target_info)
                };

                drsc.rendev->device->updateDescriptorSets(descriptor_writes, nullptr);
            }
        }
    }
}

void ComputeSvoRaytracer::upload_uniform_buffers() {
    for (size_t devidx = 0; devidx < this->device_resources.size(); ++devidx) {
        auto& drsc = this->device_resources[devidx];
        const auto& rendev = *drsc.rendev;
        const auto& device = rendev.device;
        const uint32_t outputs = static_cast<uint32_t>(rendev.outputs);

        auto staging_buffer = Buffer<UniformBuffer>(
            device,
            outputs,
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        UniformBuffer* uniforms = staging_buffer.map(0, outputs);

        for (size_t outputidx = 0; outputidx < drsc.output_resources.size(); ++outputidx) {
            auto& orsc = drsc.output_resources[outputidx];

            uniforms[outputidx].output_region = orsc.region;
            uniforms[outputidx].display_region = this->display_region;
        }

        staging_buffer.unmap();

        const auto copy_info = vk::BufferCopy{
            0,
            0,
            sizeof(UniformBuffer) * outputs
        };

        rendev.graphics_command_pool.one_time_submit([&](vk::CommandBuffer cmd_buf) {
            cmd_buf.copyBuffer(staging_buffer.get(), drsc.uniform_buffer.get(), copy_info);
        });
    }
}

void ComputeSvoRaytracer::upload_tree_buffers() {
    const Span<Octree::Node> nodes = this->model.data();

    const auto copy_info = vk::BufferCopy{
        0,
        0,
        nodes.size() * sizeof(Octree::Node)
    };

    for (auto& drsc : this->device_resources) {
        const auto& rendev = *drsc.rendev;
        const auto& device = rendev.device;

        auto staging_buffer = Buffer<Octree::Node>(
            device,
            nodes.size(),
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        Octree::Node* staging_nodes = staging_buffer.map(0, nodes.size());

        for (size_t i = 0; i < nodes.size(); ++i) {
            staging_nodes[i] = nodes[i];
        }

        staging_buffer.unmap();

        rendev.graphics_command_pool.one_time_submit([&](vk::CommandBuffer cmd_buf) {
            cmd_buf.copyBuffer(staging_buffer.get(), drsc.tree_buffer.get(), copy_info);
        });
    }
}

void ComputeSvoRaytracer::calculate_display_rect() {
    bool first = true;
    const size_t n = this->display->num_render_devices();
    for (size_t i = 0; i < n; ++i) {
        const auto& rendev = this->display->render_device(i);
        for (size_t j = 0; j < rendev.outputs; ++j) {
            Output* output = this->display->output(i, j);
            if (first) {
                this->display_region = output->region();
                first = false;
            } else {
                this->display_region = rect_union(this->display_region, output->region());
            }
        }
    }
}
