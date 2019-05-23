#include "graphics/core/Swapchain.h"
#include "core/Error.h"

namespace {
    constexpr const auto PREFERRED_FORMAT = vk::SurfaceFormatKHR{vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};

    vk::SurfaceFormatKHR find_surface_format(vk::PhysicalDevice physical, vk::SurfaceKHR surface) {
        const auto formats = physical.getSurfaceFormatsKHR(surface);

        // Can we pick any format?
        if (formats.size() == 1 && formats.front().format == vk::Format::eUndefined)
            return PREFERRED_FORMAT;

        // Check if the preferred format is available
        if (std::find(formats.begin(), formats.end(), PREFERRED_FORMAT) != formats.end())
            return PREFERRED_FORMAT;

        // Pick any format
        return formats.front();
    }

    vk::PresentModeKHR find_present_mode(vk::PhysicalDevice physical, vk::SurfaceKHR surface) {
        const auto present_modes = physical.getSurfacePresentModesKHR(surface);

        for (const auto mode : {vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eImmediate}) {
            if (std::find(present_modes.begin(), present_modes.end(), vk::PresentModeKHR::eMailbox) != present_modes.end())
                return mode;
        }

        // Double buffering, guaranteed to be available but not always supported
        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D find_extent(const vk::SurfaceCapabilitiesKHR& caps, vk::Extent2D surface_extent) {
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

Swapchain::Swapchain(Device& device, Queue graphics_queue, vk::SurfaceKHR surface, vk::Extent2D surface_extent):
    device(&device),
    graphics_queue(graphics_queue),
    surface(surface) {
    this->recreate(surface_extent);
}

void Swapchain::recreate(vk::Extent2D surface_extent) {
    auto dev = this->device->get();
    dev.waitIdle();

    const auto usage_flags = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage;

    const auto caps = this->device->physical_device().getSurfaceCapabilitiesKHR(this->surface);
    this->format = find_surface_format(this->device->physical_device(), surface);
    this->present_mode = find_present_mode(this->device->physical_device(), surface);
    this->extent = find_extent(caps, surface_extent);

    if ((caps.supportedUsageFlags & usage_flags) != usage_flags) {
        throw Error("Surface does not support ColorAttachment and/or Storage flags");
    }

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
            usage_flags,
            vk::SharingMode::eExclusive,
            1,
            &this->graphics_queue.queue_family_index(),
            caps.currentTransform,
            vk::CompositeAlphaFlagBitsKHR::eOpaque,
            present_mode,
            true,
            this->swapchain.get() // Returns nullptr if recreating, or else returns the old swapchain
        );

        this->swapchain = dev.createSwapchainKHRUnique(create_info);
    }

    // Retrieve the swapchains' images
    {
        auto swapchain_images = dev.getSwapchainImagesKHR(this->swapchain.get());
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
            this->images[i].view = dev.createImageViewUnique(create_info);
        }
    }

    // Create the synchronization objects
    {
        for (size_t i = 0; i < this->images.size(); ++i) {
            this->images[i].image_acquired = dev.createSemaphoreUnique(vk::SemaphoreCreateInfo());
            this->images[i].render_finished = dev.createSemaphoreUnique(vk::SemaphoreCreateInfo());
            this->images[i].frame_fence = dev.createFenceUnique({});
        }
    }

    this->current_image_acquired_sem = dev.createSemaphoreUnique(vk::SemaphoreCreateInfo());

    vk::Result result = dev.acquireNextImageKHR(
        this->swapchain.get(),
        std::numeric_limits<uint64_t>::max(),
        this->current_image_acquired_sem.get(),
        vk::Fence(),
        &this->current_image_index
    );

    vk::createResultValue(result, __PRETTY_FUNCTION__);

    std::swap(
        this->images[this->current_image_index].image_acquired.get(),
        this->current_image_acquired_sem.get()
    );
}

vk::Result Swapchain::swap_buffers() {
    auto& current_image = this->current_image();
    auto dev = this->device->get();

    dev.waitForFences(current_image.frame_fence.get(), true, std::numeric_limits<uint64_t>::max());
    dev.resetFences(current_image.frame_fence.get());

    auto present_info = vk::PresentInfoKHR(
        1,
        &current_image.render_finished.get(),
        1,
        &this->swapchain.get(),
        &this->current_image_index
    );

    this->graphics_queue->presentKHR(present_info);

    vk::Result result = dev.acquireNextImageKHR(
        this->swapchain.get(),
        std::numeric_limits<uint64_t>::max(),
        this->current_image_acquired_sem.get(),
        vk::Fence(),
        &this->current_image_index
    );

    if (result != vk::Result::eSuccess)
        return result;

    std::swap(
        this->images[this->current_image_index].image_acquired.get(),
        this->current_image_acquired_sem.get()
    );

    return vk::Result::eSuccess;
}
