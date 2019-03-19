#ifndef _XENODON_RENDER_TESTRENDERER_H
#define _XENODON_RENDER_TESTRENDERER_H

#include <vulkan/vulkan.hpp>
#include "graphics/Device.h"

class TestRenderer {
    Device* device;
    vk::Extent2D extent;

    vk::UniquePipelineLayout layout;
    vk::UniqueRenderPass render_pass;
    vk::UniquePipeline pipeline;

public:
    TestRenderer(Device& device, vk::Extent2D extent, vk::AttachmentDescription target);
    TestRenderer(TestRenderer&&) = default;
    TestRenderer& operator=(TestRenderer&&) = default;

    void present(vk::CommandBuffer cmd, vk::Framebuffer target);
    vk::RenderPass final_render_pass() const;

private:
    void init_render_pass(vk::AttachmentDescription target);
};

#endif
