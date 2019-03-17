#include "graphics/support.h"
#include <algorithm>
#include <cstring>

bool check_extension_support(vk::PhysicalDevice gpu, Span<const char* const> extensions) {
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

bool check_surface_support(vk::PhysicalDevice gpu, vk::SurfaceKHR surface) {
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
