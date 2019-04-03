#ifndef _XENODON_GRAPHICS_CORE_SWAPCHAIN_H
#define _XENODON_GRAPHICS_CORE_SWAPCHAIN_H

#include <vector>
#include <vulkan/vulkan.hpp>
#include "graphics/core/Device.h"
#include "graphics/core/Queue.h"

// Implementation inspired by
// https://chromium.googlesource.com/chromium/src/gpu/+/45b0f5183d399bed91c38c1ac869b7ce05f72d14/vulkan/vulkan_swap_chain.cc
class Swapchain2 {
public:
    struct SwapImageResources {
        vk::Image image;
        vk::UniqueImageView view;
        vk::UniqueSemaphore image_acquired;
        vk::UniqueSemaphore render_finished;
        vk::UniqueFence frame_fence;
    };

private:
    Device2* device;
    Queue2 graphics_queue;

    vk::SurfaceKHR surface;
    vk::SurfaceFormatKHR format;
    vk::PresentModeKHR present_mode;
    vk::Extent2D extent;
    vk::UniqueSwapchainKHR swapchain;
    std::vector<SwapImageResources> images;
    uint32_t current_image_index;
    vk::UniqueSemaphore current_image_acquired_sem;

public:
    Swapchain2(Device2& device, Queue2 graphics_queue, vk::SurfaceKHR surface, vk::Extent2D surface_extent);
    void recreate(vk::Extent2D surface_extent);
    vk::Result swap_buffers();

    vk::PresentModeKHR surface_present_mode() const {
        return this->present_mode;
    }

    vk::Extent2D surface_extent() const {
        return this->extent;
    }

    vk::SurfaceFormatKHR surface_format() const {
        return this->format;
    }

    uint32_t num_images() const {
        return static_cast<uint32_t>(this->images.size());
    }

    uint32_t current_index() const {
        return this->current_image_index;
    }

    SwapImageResources& current_image() {
        return this->images[this->current_image_index];
    }

    const SwapImageResources& current_image() const {
        return this->images[this->current_image_index];
    }

    SwapImageResources& image(uint32_t index) {
        return this->images[index];
    }

    const SwapImageResources& image(uint32_t index) const {
        return this->images[index];
    }
};

#endif
