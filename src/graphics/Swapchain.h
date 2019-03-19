#ifndef _XENODON_GRAPHICS_SWAPCHAIN_H
#define _XENODON_GRAPHICS_SWAPCHAIN_H

#include <vector>
#include <vulkan/vulkan.hpp>
#include "graphics/Device.h"

// Implementation inspired by
// https://chromium.googlesource.com/chromium/src/gpu/+/45b0f5183d399bed91c38c1ac869b7ce05f72d14/vulkan/vulkan_swap_chain.cc
class Swapchain {
public:
    struct SwapchainImage {
        vk::Image image;
        vk::UniqueImageView image_view;
        vk::UniqueCommandBuffer command_buffer;
        vk::UniqueSemaphore image_acquired;
        vk::UniqueSemaphore render_finished;
        vk::UniqueFence fence;
    };

private:
    Device* device;
    vk::SurfaceKHR surface;
    vk::Format format;
    vk::Extent2D extent;
    vk::UniqueSwapchainKHR swapchain;
    std::vector<SwapchainImage> images;
    uint32_t current_image_index;
    vk::UniqueSemaphore current_image_acquired_sem;

public:
    Swapchain(Device& device, vk::SurfaceKHR surface, vk::Extent2D surface_extent);
    void recreate(vk::Extent2D surface_extent);
    vk::Result swap_buffers();
    std::vector<vk::UniqueFramebuffer> create_framebuffers(vk::RenderPass pass);

    vk::Extent2D surface_extent() const {
        return this->extent;    
    }

    vk::Format surface_format() const {
        return this->format;
    }

    uint32_t num_images() const {
        return static_cast<uint32_t>(this->images.size());
    }

    uint32_t current_index() const {
        return this->current_image_index;
    }

    SwapchainImage& current_image() {
        return this->images[this->current_image_index];
    }

    const SwapchainImage& current_image() const {
        return this->images[this->current_image_index];
    }

    SwapchainImage& image(uint32_t index) {
        return this->images[index];
    }

    const SwapchainImage& image(uint32_t index) const {
        return this->images[index];
    }
};

#endif
