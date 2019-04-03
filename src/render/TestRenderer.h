#ifndef _XENODON_RENDER_TESTRENDERER_H
#define _XENODON_RENDER_TESTRENDERER_H

#include <vulkan/vulkan.hpp>
#include "graphics/core/Device.h"
#include "graphics/memory/Buffer.h"
#include "math/Vec.h"

class TestRenderer {
    struct OutputRegion {
        Vec2F min;
        Vec2F max;
        Vec2F resolution;
    };

    const Device& device;
    vk::Rect2D region;

    vk::UniqueDescriptorSetLayout descriptor_layout;
    vk::UniquePipelineLayout layout;
    vk::UniqueRenderPass render_pass;
    vk::UniquePipeline pipeline;
    Buffer<OutputRegion> output_region;

public:
    TestRenderer(const Device& device, vk::Rect2D region, vk::AttachmentDescription output_attachment);

    void present(vk::CommandBuffer cmd, vk::Framebuffer target);
    vk::RenderPass final_render_pass() const;
};

#endif
