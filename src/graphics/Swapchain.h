#ifndef _XENODON_GRAPHICS_SWAPCHAIN_H
#define _XENODON_GRAPHICS_SWAPCHAIN_H

#include <vector>
#include <functional>
#include <vulkan/vulkan.hpp>
#include "graphics/Device.h"

struct SwapImage {
    vk::CommandBuffer command_buffer;
    vk::Image image;
    vk::ImageView view;
};

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

    using PresentCallback = std::function<void(uint32_t, const SwapImage&)>;

private:
    Device* device;
    vk::SurfaceKHR surface;
    vk::SurfaceFormatKHR format;
    vk::PresentModeKHR present_mode;
    vk::Extent2D extent;
    vk::UniqueSwapchainKHR swapchain;
    std::vector<SwapchainImage> images;
    uint32_t image_index;

public:
    Swapchain(Device& device, vk::SurfaceKHR surface, vk::Extent2D surface_extent);
    void recreate(vk::Extent2D surface_extent);
    vk::Result swap_buffers();
    std::vector<vk::UniqueFramebuffer> create_framebuffers(vk::RenderPass pass);
    vk::Result present(PresentCallback f);

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

    SwapImage image(uint32_t index) const {
        return {
            this->images[index].command_buffer.get(),
            this->images[index].image,
            this->images[index].image_view.get()
        };
    }
};

#endif
