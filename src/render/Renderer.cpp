#include "render/Renderer.h"
#include <array>
#include <algorithm>
#include <iterator>
#include <utility>
#include "utility/rect_union.h"
#include "core/Logger.h"
#include "graphics/shader/Shader.h"
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

Renderer::Renderer(Display* display, const RenderAlgorithm* algorithm):
    display(display),
    algorithm(algorithm) {

    std::copy(STANDARD_BINDINGS.begin(), STANDARD_BINDINGS.end(), std::back_inserter(this->bindings));

    const auto custom_bindings = algorithm->bindings();
    std::copy(custom_bindings.begin(), custom_bindings.end(), std::back_inserter(this->bindings));

    this->calculate_display_rect();
    this->create_resources();
}

void Renderer::recreate(size_t device, size_t output) {
    LOGGER.log("Unimplemented method recreate");
}

void Renderer::render() {
    LOGGER.log("Unimplemented method render");
}

void Renderer::create_resources() {
    const size_t num_devices = this->display->num_render_devices();
    this->device_resources.clear();
    this->device_resources.reserve(num_devices);

    for (size_t i = 0; i < num_devices; ++i) {
        const auto& rendev = this->display->render_device(i);
        const auto& device = rendev.device;
        const uint32_t outputs = static_cast<uint32_t>(rendev.outputs);

        auto instance = this->algorithm->instantiate(device);

        auto descr_set_layout = device->createDescriptorSetLayoutUnique({
            {},
            static_cast<uint32_t>(this->bindings.size()),
            this->bindings.data()
        });

        const auto shader = Shader(device, vk::ShaderStageFlagBits::eCompute, this->algorithm->shader());

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

        auto uniform_buffer = Buffer<UniformBuffer>(
            device,
            outputs,
            vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

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
            std::move(instance),

            std::move(descr_set_layout),
            vk::UniqueDescriptorPool(),
            std::vector<vk::DescriptorSet>(),

            std::move(pipeline_layout),

            std::move(uniform_buffer),

            std::move(output_resources)
        });
    }
}

void Renderer::create_descriptor_sets() {
    for (size_t devidx = 0; devidx < this->device_resources.size(); ++devidx) {
        auto& drsc = this->device_resources[devidx];
        const auto& device = drsc.rendev->device;

        uint32_t total_images = 0;
        for (const auto& orsc : drsc.output_resources) {
            total_images += orsc.output->num_swap_images();
        }

        drsc.descriptor_pool = create_descriptor_pool(device, total_images);

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

    for (const auto& binding : this->bindings) {
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

void Renderer::calculate_display_rect() {
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
