#ifndef _XENODON_RENDER_COMPUTESVORAYTRACER_H
#define _XENODON_RENDER_COMPUTESVORAYTRACER_H

#include <chrono>
#include <vulkan/vulkan.hpp>
#include "backend/Display.h"
#include "backend/RenderDevice.h"
#include "graphics/memory/Buffer.h"
#include "model/Octree.h"
#include "utility/Span.h"

class ComputeSvoRaytracer {
    struct UniformBuffer {
        vk::Rect2D output_region;
        vk::Rect2D display_region;
    };

    struct OutputResources {
        Output* output;

        vk::Rect2D region;
        vk::UniquePipeline pipeline;

        Span<vk::DescriptorSet> descriptor_sets;
        std::vector<vk::UniqueCommandBuffer> command_buffers;
    };

    struct DeviceResources {
        const RenderDevice* rendev;

        vk::UniqueDescriptorSetLayout descriptor_set_layout;
        vk::UniqueDescriptorPool descriptor_pool;
        std::vector<vk::DescriptorSet> descriptor_sets;

        vk::UniquePipelineLayout pipeline_layout;

        Buffer<UniformBuffer> uniform_buffer;
        Buffer<Octree::Node> tree_buffer;

        std::vector<OutputResources> output_resources;
    };

    Display* display;
    const Octree& model;
    std::chrono::system_clock::time_point start;
    vk::Rect2D display_region;
    std::vector<DeviceResources> device_resources;

public:
    ComputeSvoRaytracer(Display* display, const Octree& model);
    void recreate(size_t device, size_t output);
    void render();

private:
    void create_resources();
    void create_descriptor_sets();
    void create_command_buffers();
    void update_descriptor_sets();
    void upload_uniform_buffers();
    void upload_tree_buffers();
    void calculate_display_rect();
};

#endif
