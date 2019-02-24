#ifndef _XENODON_INTERACTIVE_SURFACEINFO_H
#define _XENODON_INTERACTIVE_SURFACEINFO_H

#include <cstdint>
#include <vulkan/vulkan.hpp>

struct SurfaceInfo {
    vk::SurfaceCapabilitiesKHR caps;
    vk::SurfaceFormatKHR surface_format;
    vk::PresentModeKHR present_mode;
    vk::Extent2D extent;
    vk::AttachmentDescription attachment_description;
    uint32_t image_count;

    SurfaceInfo(vk::PhysicalDevice physical, vk::SurfaceKHR surface, vk::Extent2D window_size);
};

#endif
