#ifndef _XENODON_RENDER_SVORAYTRACER_H
#define _XENODON_RENDER_SVORAYTRACER_H

#include <vector>
#include <chrono>
#include <vulkan/vulkan.hpp>
#include "graphics/memory/Buffer.h"
#include "backend/RenderDevice.h"
#include "backend/Display.h"
#include "backend/Output.h"
#include "render/FrameResources.h"
#include "model/Octree.h"
#include "math/Vec.h"

class SvoRaytracer {
    struct OutputRegionUbo {
        Vec2F min;
        Vec2F max;
        Vec2F offset;
        Vec2F extent;
    };

    struct OutputResources {
        Output* output;
        vk::Rect2D region;
        vk::DescriptorSet descr_set;
        vk::UniqueRenderPass render_pass;
        vk::UniquePipeline pipeline;
        FrameResources frame_resources;
    };

    struct DeviceResources {
        const RenderDevice* rendev;
        vk::UniqueDescriptorSetLayout descr_set_layout;
        vk::UniqueDescriptorPool descr_pool;
        vk::UniquePipelineLayout pipeline_layout;
        Buffer<OutputRegionUbo> output_region_buffer;
        Buffer<Octree::Node> tree_buffer;
        std::vector<OutputResources> output_resources;
    };

    Display* display;
    std::vector<DeviceResources> device_resources;
    vk::Rect2D enclosing;
    std::chrono::system_clock::time_point start;
    const Octree& model;

public:
    SvoRaytracer(Display* display, const Octree& model);
    void render();
    void recreate(size_t device, size_t output);

private:
    void calculate_rect_union();
    void create_resources();
    void update_output_regions();
    void update_tree_buffers();
};

#endif
