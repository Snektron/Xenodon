#ifndef _XENODON_INTERACTIVE_SWAPCHAIN_H
#define _XENODON_INTERACTIVE_SWAPCHAIN_H

#include <vector>
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include "render/DeviceContext.h"

struct Swapchain {
    struct Frame {
        vk::Image image;
        vk::UniqueImageView view;
        vk::UniqueFramebuffer framebuffer;
        vk::UniqueCommandBuffer command_buffer;
    };

    DeviceContext& device_context;
    vk::SurfaceKHR surface;
    vk::UniqueSwapchainKHR swapchain;
    vk::Format format;
    vk::Extent2D extent;
    std::vector<Frame> frames;
    uint32_t active_frame_index;

    Swapchain(DeviceContext& device_context, vk::SurfaceKHR surface, vk::Extent2D window_size, vk::RenderPass render_pass);
    void recreate(vk::Extent2D window_size, vk::RenderPass render_pass);
    vk::Result acquire_next_image(vk::Semaphore image_available = vk::Semaphore(nullptr));
    void present(vk::Semaphore render_finished = vk::Semaphore(nullptr));
    Frame& active_frame();
};

#endif
