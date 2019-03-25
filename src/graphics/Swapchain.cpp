#include "graphics/Swapchain.h"
#include <limits>
#include <algorithm>
#include <array>

namespace {
    constexpr const auto PREFERRED_FORMAT = vk::SurfaceFormatKHR{vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};

    vk::SurfaceFormatKHR pick_surface_format(vk::PhysicalDevice physical, vk::SurfaceKHR surface) {
        auto formats = physical.getSurfaceFormatsKHR(surface);

        // Can we pick any format?
        if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined)
            return PREFERRED_FORMAT;

        // Check if the preferred format is available
        if (std::find(formats.begin(), formats.end(), PREFERRED_FORMAT) != formats.end())
            return PREFERRED_FORMAT;

        // Pick any format
        return formats[0];
    }

    vk::PresentModeKHR pick_present_mode(vk::PhysicalDevice physical, vk::SurfaceKHR surface) {
        auto present_modes = physical.getSurfacePresentModesKHR(surface);

        // check for triple buffering support
        if (std::find(present_modes.begin(), present_modes.end(), vk::PresentModeKHR::eMailbox) != present_modes.end())
            return vk::PresentModeKHR::eMailbox;

        // Immediate mode
        if (std::find(present_modes.begin(), present_modes.end(), vk::PresentModeKHR::eImmediate) != present_modes.end())
            return vk::PresentModeKHR::eImmediate;

        // Double buffering, guaranteed to be available but not always supported
        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D pick_extent(const vk::SurfaceCapabilitiesKHR& caps, vk::SurfaceKHR surface, vk::Extent2D surface_extent) {
        if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return caps.currentExtent;
        } else {
            return {
                std::clamp(caps.minImageExtent.width, caps.maxImageExtent.width, surface_extent.width),
                std::clamp(caps.minImageExtent.height, caps.maxImageExtent.height, surface_extent.height),
            };
        }
    }
}

Swapchain::Swapchain(Device& device, vk::SurfaceKHR surface, vk::Extent2D surface_extent):
    device(&device),
    surface(surface) {
    this->recreate(extent);
}

void Swapchain::recreate(vk::Extent2D surface_extent) {
    auto caps = this->device->physical.getSurfaceCapabilitiesKHR(this->surface);
    this->format = pick_surface_format(this->device->physical, surface);
    this->present_mode = pick_present_mode(this->device->physical, surface);
    this->extent = pick_extent(caps, surface, extent);

    // Create the swapchain itself
    {
        uint32_t image_count = caps.minImageCount + 1;
        if (caps.maxImageCount > 0)
            image_count = std::min(caps.maxImageCount, image_count);

        auto create_info = vk::SwapchainCreateInfoKHR(
            {},
            this->surface,
            image_count,
            this->format.format,
            this->format.colorSpace,
            this->extent,
            1,
            vk::ImageUsageFlagBits::eColorAttachment,
            vk::SharingMode::eExclusive,
            1,
            &this->device->present.family_index,
            caps.currentTransform,
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            present_mode,
            true,
            this->swapchain.get() // Returns nullptr if recreating, or else returns the old swapchain
        );

        if (this->device->graphics.family_index != this->device->present.family_index) {
            auto queue_indices = std::array{
                this->device->graphics.family_index,
                this->device->present.family_index
            };

            create_info.imageSharingMode = vk::SharingMode::eConcurrent;
            create_info.queueFamilyIndexCount = static_cast<uint32_t>(queue_indices.size());
            create_info.pQueueFamilyIndices = queue_indices.data();
        }

        this->swapchain = this->device->logical->createSwapchainKHRUnique(create_info);
    }

    // Retrieve the swapchains' images
    {
        auto swapchain_images = this->device->logical->getSwapchainImagesKHR(this->swapchain.get());
        this->images.clear();
        this->images.resize(swapchain_images.size());

        for (size_t i = 0; i < this->images.size(); ++i) {
            this->images[i].image = swapchain_images[i];
        }
    }

    // Create the image views
    {
        auto sub_resource_range = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
        
        auto component_mapping = vk::ComponentMapping(
            vk::ComponentSwizzle::eR,
            vk::ComponentSwizzle::eG,
            vk::ComponentSwizzle::eB,
            vk::ComponentSwizzle::eA
        );

        auto create_info = vk::ImageViewCreateInfo(
            {},
            vk::Image(),
            vk::ImageViewType::e2D,
            this->format.format,
            component_mapping,
            sub_resource_range
        );

        for (size_t i = 0; i < this->images.size(); ++i) {
            create_info.image = this->images[i].image;
            this->images[i].image_view = this->device->logical->createImageViewUnique(create_info);
        }
    }

    // Create the command buffers
    {   
        auto command_buffers_info = vk::CommandBufferAllocateInfo(this->device->graphics_command_pool.get());
        command_buffers_info.commandBufferCount = static_cast<uint32_t>(this->images.size());
        auto command_buffers = this->device->logical->allocateCommandBuffersUnique(command_buffers_info);

        for (size_t i = 0; i < this->images.size(); ++i) {
            this->images[i].command_buffer = std::move(command_buffers[i]);
        }
    }

    // Create the synchronization objects
    {
        for (size_t i = 0; i < this->images.size(); ++i) {
            this->images[i].image_acquired = this->device->logical->createSemaphoreUnique(vk::SemaphoreCreateInfo());
            this->images[i].render_finished = this->device->logical->createSemaphoreUnique(vk::SemaphoreCreateInfo());
            this->images[i].fence = this->device->logical->createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
        }
    }

    this->image_index = 0;
}

vk::Result Swapchain::present(PresentCallback f) {
    const auto fence = this->images[this->image_index].fence.get();
    this->device->logical->waitForFences(fence, true, std::numeric_limits<uint64_t>::max());
    this->device->logical->resetFences(fence);

    const auto image_acquired = this->images[this->image_index].image_acquired.get();
    const auto render_finished = this->images[this->image_index].render_finished.get();

    vk::Result result = this->device->logical->acquireNextImageKHR(
        this->swapchain.get(),
        std::numeric_limits<uint64_t>::max(),
        image_acquired,
        vk::Fence(),
        &this->image_index
    );

    if (result != vk::Result::eSuccess)
        return result;

    f(
        this->image_index,
        this->image(this->image_index)
    );

    auto wait_stages = std::array{
        vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput)
    };

    auto submit_info = vk::SubmitInfo(
        1,
        &image_acquired,
        wait_stages.data(),
        1,
        &this->images[this->image_index].command_buffer.get(),
        1,
        &render_finished
    );

    this->device->graphics.queue.submit(1, &submit_info, fence);

    auto present_info = vk::PresentInfoKHR(
        1,
        &render_finished,
        1,
        &this->swapchain.get(),
        &this->image_index
    );

    this->device->present.queue.presentKHR(&present_info);

    return vk::Result::eSuccess;
}

std::vector<vk::UniqueFramebuffer> Swapchain::create_framebuffers(vk::RenderPass pass) {
    auto vec = std::vector<vk::UniqueFramebuffer>(this->images.size());

    auto create_info = vk::FramebufferCreateInfo(
        {},
        pass,
        1,
        nullptr,
        this->extent.width,
        this->extent.height,
        1
    );

    for (size_t i = 0; i < this->images.size(); ++i) {
        create_info.pAttachments = &this->images[i].image_view.get();
        vec[i] = this->device->logical->createFramebufferUnique(create_info);
    }

    return vec;
}
