#include "graphics/utility.h"
#include <algorithm>
#include <cstring>

bool gpu_supports_extensions(vk::PhysicalDevice gpu, Span<const char* const> extensions) {
    auto extension_properties = gpu.enumerateDeviceExtensionProperties();

    auto it = extensions.begin();
    bool supported = true;
    for (; it != extensions.end() && supported; ++it) {
        auto cmp_ext = [&](auto& ext) {
            return std::strcmp(*it, ext.extensionName) == 0;
        };

        supported = extension_properties.cend() != std::find_if(
            extension_properties.cbegin(),
            extension_properties.cend(), cmp_ext
        );
    }

    return supported;
}

bool gpu_supports_surface(vk::PhysicalDevice gpu, vk::SurfaceKHR surface) {
    uint32_t format_count, present_mode_count;

    vk::createResultValue(
        gpu.getSurfaceFormatsKHR(
            surface,
            &format_count,
            static_cast<vk::SurfaceFormatKHR*>(nullptr)
        ),
        __PRETTY_FUNCTION__
    );

    vk::createResultValue(
        gpu.getSurfacePresentModesKHR(
            surface,
            &present_mode_count,
            static_cast<vk::PresentModeKHR*>(nullptr)
        ),
        __PRETTY_FUNCTION__
    );

    return format_count != 0 && present_mode_count != 0;
}

std::optional<uint32_t> pick_graphics_queue(vk::PhysicalDevice gpu, Span<const vk::SurfaceKHR> surfaces) {
    auto surfaces_supported = [gpu, &surfaces](uint32_t i) {
        for (auto& surface : surfaces) {
            if (!gpu.getSurfaceSupportKHR(i, surface))
                return false;
        }

        return true;
    };

    auto queue_families = gpu.getQueueFamilyProperties();
    uint32_t num_queues = static_cast<uint32_t>(queue_families.size());
    for (uint32_t i = 0; i < num_queues; ++i) {
        if ((queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics) && surfaces_supported(i))
            return i;
    }

    return std::nullopt;
}
