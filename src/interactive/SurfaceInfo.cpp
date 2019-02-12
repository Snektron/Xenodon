#include "interactive/SurfaceInfo.h"
#include <limits>
#include <algorithm>

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
        // if (std::find(present_modes.begin(), present_modes.end(), vk::PresentModeKHR::eMailbox) != present_modes.end())
            // return vk::PresentModeKHR::eMailbox;

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

SurfaceInfo::SurfaceInfo(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface, vk::Extent2D window_size):
    caps(physical_device.getSurfaceCapabilitiesKHR(surface)),
    surface_format(pick_surface_format(physical_device, surface)),
    present_mode(pick_present_mode(physical_device, surface)),
    extent(pick_extent(this->caps, surface, window_size)) {

    this->attachment_description = vk::AttachmentDescription({}, this->surface_format.format);
    this->attachment_description.loadOp = vk::AttachmentLoadOp::eClear;
    this->attachment_description.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    this->image_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0)
        this->image_count = std::min(caps.maxImageCount, image_count);
}