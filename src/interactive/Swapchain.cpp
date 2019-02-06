#include "interactive/Swapchain.h"
#include <algorithm>
#include <array>
#include <utility>
#include <limits>
#include <cstddef>

namespace {
    constexpr const auto PREFERRED_FORMAT = vk::SurfaceFormatKHR{vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};

    vk::SurfaceFormatKHR pick_surface_format(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface) {
        auto formats = physical_device.getSurfaceFormatsKHR(surface);

        // Can we pick any format?
        if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined)
            return PREFERRED_FORMAT;

        // Check if the preferred format is available
        if (std::find(formats.begin(), formats.end(), PREFERRED_FORMAT) != formats.end())
            return PREFERRED_FORMAT;

        // Pick any format
        return formats[0];
    }

    vk::PresentModeKHR pick_present_mode(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface) {
        auto present_modes = physical_device.getSurfacePresentModesKHR(surface);

        // check for triple buffering support
        if (std::find(present_modes.begin(), present_modes.end(), vk::PresentModeKHR::eMailbox) != present_modes.end())
            return vk::PresentModeKHR::eMailbox;

        // Immediate mode
        if (std::find(present_modes.begin(), present_modes.end(), vk::PresentModeKHR::eImmediate) != present_modes.end())
            return vk::PresentModeKHR::eImmediate;

        // Double buffering, guaranteed to be available but not always supported
        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D pick_extent(const vk::SurfaceCapabilitiesKHR& caps, vk::SurfaceKHR surface, vk::Extent2D window_size) {
        if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return caps.currentExtent;
        } else {
            return {
                std::clamp(caps.minImageExtent.width, caps.maxImageExtent.width, window_size.width),
                std::clamp(caps.minImageExtent.height, caps.maxImageExtent.height, window_size.height),
            };
        }
    }
}

Swapchain::Swapchain(DeviceContext& device_context, vk::SurfaceKHR surface, vk::Extent2D window_size, vk::RenderPass render_pass):
    device_context(device_context),
    surface(surface) {
    this->recreate(window_size, render_pass);
}

void Swapchain::recreate(vk::Extent2D window_size, vk::RenderPass render_pass) {
    // Create the swapchain
    {
        const auto caps = this->device_context.physical_device.getSurfaceCapabilitiesKHR(this->surface);
        const auto surface_format = pick_surface_format(this->device_context.physical_device, this->surface);
        const auto present_mode = pick_present_mode(this->device_context.physical_device, this->surface);
        this->extent = pick_extent(caps, this->surface, window_size);

        uint32_t image_count = caps.minImageCount + 1;
        if (caps.maxImageCount > 0)
            image_count = std::min(caps.maxImageCount, image_count);

        auto create_info = vk::SwapchainCreateInfoKHR(
            {},
            this->surface,
            image_count,
            surface_format.format,
            surface_format.colorSpace,
            this->extent,
            1,
            vk::ImageUsageFlagBits::eColorAttachment,
            vk::SharingMode::eExclusive,
            0,
            nullptr,
            caps.currentTransform,
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            present_mode,
            true,
            this->swapchain.get() // Returns nullptr if recreating, or else returns the old swapchain
        );

        if (this->device_context.graphics.family_index != this->device_context.present.family_index) {
            auto queue_indices = std::array{device_context.graphics.family_index, device_context.present.family_index};
            create_info.imageSharingMode = vk::SharingMode::eConcurrent;
            create_info.queueFamilyIndexCount = static_cast<uint32_t>(queue_indices.size());
            create_info.pQueueFamilyIndices = queue_indices.data();
        }

        this->swapchain = this->device_context.device.createSwapchainKHRUnique(create_info);
        this->format = surface_format.format;
    }

    // Retrieve the swapchain images
    {
        auto swapchain_images = this->device_context.device.getSwapchainImagesKHR(this->swapchain.get());
        this->frames.resize(swapchain_images.size());

        for (size_t i = 0; i < this->frames.size(); ++i) {
            this->frames[i].image = swapchain_images[i];
        }
    }

    // Create the views
    {
        auto component_mapping = vk::ComponentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA);
        auto sub_resource_range = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

        for (size_t i = 0; i < this->frames.size(); ++i) {
            auto create_info = vk::ImageViewCreateInfo(
                {},
                this->frames[i].image,
                vk::ImageViewType::e2D,
                this->format,
                component_mapping,
                sub_resource_range
            );

            this->frames[i].view = this->device_context.device.createImageViewUnique(create_info);
        }
    }
    
    // Create the framebuffers
    {
        for (size_t i = 0; i < this->frames.size(); ++i) {
            auto create_info = vk::FramebufferCreateInfo(
                {},
                render_pass,
                1,
                &this->frames[i].view.get(),
                this->extent.width,
                this->extent.height,
                1
            );

            this->frames[i].framebuffer = this->device_context.device.createFramebufferUnique(create_info);
        }
    }

    // Create the command buffers
    {
        auto command_buffers_info = vk::CommandBufferAllocateInfo(this->device_context.graphics_command_pool.get());
        command_buffers_info.commandBufferCount = static_cast<uint32_t>(this->frames.size());
        auto command_buffers = this->device_context.device.allocateCommandBuffersUnique(command_buffers_info);

        for (size_t i = 0; i < this->frames.size(); ++i) {
            this->frames[i].command_buffer = std::move(command_buffers[i]);
        }
    }
}

vk::Result Swapchain::acquire_next_image(vk::Semaphore image_available) {
    return this->device_context.device.acquireNextImageKHR(
        this->swapchain.get(),
        std::numeric_limits<uint64_t>::max(),
        image_available,
        vk::Fence(),
        &this->active_frame_index
    );
}

void Swapchain::present(vk::Semaphore render_finished) {
    auto present_info = vk::PresentInfoKHR(
        1,
        &render_finished,
        1,
        &this->swapchain.get(),
        &this->active_frame_index
    );

    device_context.present.queue.presentKHR(&present_info);
}

Swapchain::Frame& Swapchain::active_frame() {
    return this->frames[this->active_frame_index];
}