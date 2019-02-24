#include "interactive/Swapchain.h"
#include <array>
#include <iostream>
#include <utility>
#include <cstddef>

Swapchain::Swapchain(DeviceContext& device_context, vk::SurfaceKHR surface, SurfaceInfo& sinf, vk::RenderPass render_pass):
    device_context(device_context),
    surface(surface) {
    this->recreate(sinf, render_pass);
}

void Swapchain::recreate(SurfaceInfo& sinf, vk::RenderPass render_pass) {
    // Create the swapchain
    {
        this->extent = sinf.extent;

        auto create_info = vk::SwapchainCreateInfoKHR(
            {},
            this->surface,
            sinf.image_count,
            sinf.surface_format.format,
            sinf.surface_format.colorSpace,
            this->extent,
            1,
            vk::ImageUsageFlagBits::eColorAttachment,
            vk::SharingMode::eExclusive,
            0,
            nullptr,
            sinf.caps.currentTransform,
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            sinf.present_mode,
            true,
            this->swapchain.get() // Returns nullptr if recreating, or else returns the old swapchain
        );

        if (this->device_context.graphics.family_index != this->device_context.present.family_index) {
            auto queue_indices = std::array{device_context.graphics.family_index, device_context.present.family_index};
            create_info.imageSharingMode = vk::SharingMode::eConcurrent;
            create_info.queueFamilyIndexCount = static_cast<uint32_t>(queue_indices.size());
            create_info.pQueueFamilyIndices = queue_indices.data();
        }

        this->swapchain = this->device_context.logical->createSwapchainKHRUnique(create_info);
    }

    // Retrieve the swapchain images
    {
        auto swapchain_images = this->device_context.logical->getSwapchainImagesKHR(this->swapchain.get());
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
                sinf.surface_format.format,
                component_mapping,
                sub_resource_range
            );

            this->frames[i].view = this->device_context.logical->createImageViewUnique(create_info);
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

            this->frames[i].framebuffer = this->device_context.logical->createFramebufferUnique(create_info);
        }
    }

    // Create the command buffers
    {
        auto command_buffers_info = vk::CommandBufferAllocateInfo(this->device_context.graphics_command_pool.get());
        command_buffers_info.commandBufferCount = static_cast<uint32_t>(this->frames.size());
        auto command_buffers = this->device_context.logical->allocateCommandBuffersUnique(command_buffers_info);

        for (size_t i = 0; i < this->frames.size(); ++i) {
            this->frames[i].command_buffer = std::move(command_buffers[i]);
        }
    }
}

vk::Result Swapchain::acquire_next_image(vk::Semaphore image_available) {
    return this->device_context.logical->acquireNextImageKHR(
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