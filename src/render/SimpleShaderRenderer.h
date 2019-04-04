#ifndef _XENODON_RENDER_SIMPLESHADERRENDERER_H
#define _XENODON_RENDER_SIMPLESHADERRENDERER_H

#include <vulkan/vulkan.hpp>
#include <vector>
#include "graphics/memory/Buffer.h"
#include "backend/RenderDevice.h"
#include "backend/Display.h"
#include "backend/Output.h"
#include "render/FrameResources.h"
#include "math/Vec.h"

class SimpleShaderRenderer {
    struct OutputRegionUbo {
        Vec2F min;
        Vec2F max;
        Vec2F resolution;
    };

    struct OutputResources {
        Output* output;
        vk::Extent2D extent;
        Buffer<OutputRegionUbo> output_region_buffer;
        vk::UniqueRenderPass render_pass;
        vk::UniquePipeline pipeline;
        FrameResources frame_resources;
    };

    struct DeviceResources {
        const RenderDevice* rendev;
        vk::UniqueDescriptorSetLayout output_region_layout;
        vk::UniquePipelineLayout pipeline_layout;
        std::vector<OutputResources> output_resources;
    };

    Display* display;
    std::vector<DeviceResources> device_resources;
    vk::Rect2D enclosing;

public:
    SimpleShaderRenderer(Display* display);
    void render();
    void recreate(size_t device, size_t output);

private:
    void recalculate_enclosing_rect();
    void create_resources();
};

#endif
