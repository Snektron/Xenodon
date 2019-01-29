#ifndef _XENODON_RENDER_RENDERER_H
#define _XENODON_RENDER_RENDERER_H

#include <vulkan/vulkan.hpp>
#include <cstdint>

struct DeviceContext {
    vk::PhysicalDevice physical_device;
    vk::Device device;
};

class Renderer {
    DeviceContext& device_context;
    vk::Rect2D area; // The area of the output this renderer covers

    vk::UniquePipelineLayout layout;
    vk::UniqueRenderPass render_pass;
    vk::UniquePipeline pipeline;

public:
    Renderer(DeviceContext& device_context, vk::Rect2D area, vk::AttachmentDescription& target);
    
    void render(vk::CommandBuffer cmd, vk::Framebuffer target);
    vk::RenderPass final_render_pass() const;

private:
    void init_render_pass(vk::AttachmentDescription& target);
};

#endif
