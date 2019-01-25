#ifndef _XENODON_RENDER_PICK_UTILITY_H
#define _XENODON_RENDER_PICK_UTILITY_H

#include <vulkan/vulkan.hpp>
#include <string_view>
#include <algorithm>
#include <iterator>
#include <vector>

struct PhysicalDeviceInfo {
    vk::PhysicalDevice physical_device;
    std::vector<vk::QueueFamilyProperties> queue_family_properties;
    std::vector<vk::ExtensionProperties> extension_properties;

    PhysicalDeviceInfo(vk::PhysicalDevice physical_device);

    template <typename It>
    bool supports_extensions(It first, It last) const;

    template <typename C>
    bool supports_extensions(const C& container) const;

    bool supports_surface(vk::SurfaceKHR surface) const;
};

template <typename It>
bool PhysicalDeviceInfo::supports_extensions(It first, It last) const {
    const auto& exts = this->extension_properties;

    while (first != last) {
        auto name = std::string_view(*first);
        auto cmp_ext = [&](auto& ext) {
            return name == ext.extensionName;
        };

        if (std::find_if(exts.begin(), exts.end(), cmp_ext) == exts.end())
            return false;

        ++first;
    }

    return true;
}

template <typename C>
bool PhysicalDeviceInfo::supports_extensions(const C& container) const {
    return this->supports_extensions(std::begin(container), std::end(container));
}

#endif
