#include "graphics/core/PhysicalDevice.h"
#include <algorithm>
#include <cstring>

PhysicalDevice::PhysicalDevice(vk::PhysicalDevice physdev):
    physdev(physdev),
    props(this->physdev.getProperties()) {
}

bool PhysicalDevice::supports_extensions(Span<const char* const> extensions) const {
    for (const auto& properties : this->physdev.enumerateDeviceExtensionProperties()) {
        auto cmp_ext = [&properties](auto& ext_name) {
            return std::strcmp(properties.extensionName, ext_name);
        };

        if (!std::all_of(extensions.begin(), extensions.end(), cmp_ext)) {
            return false;
        }
    }

    return true;
}

bool PhysicalDevice::supports_surface(vk::SurfaceKHR surface) const {
    uint32_t format_count, present_mode_count;

    vk::createResultValue(
        this->physdev.getSurfaceFormatsKHR(
            surface,
            &format_count,
            static_cast<vk::SurfaceFormatKHR*>(nullptr)
        ),
        __PRETTY_FUNCTION__
    );

    vk::createResultValue(
        this->physdev.getSurfacePresentModesKHR(
            surface,
            &present_mode_count,
            static_cast<vk::PresentModeKHR*>(nullptr)
        ),
        __PRETTY_FUNCTION__
    );

    return format_count != 0 && present_mode_count != 0;
}

std::optional<uint32_t> PhysicalDevice::find_queue_family(vk::QueueFlags flags, Span<const vk::SurfaceKHR> surfaces) const {
    auto queue_families = physdev.getQueueFamilyProperties();
    uint32_t num_families = static_cast<uint32_t>(queue_families.size());
    for (uint32_t family_index = 0; family_index < num_families; ++family_index) {
        auto surface_supported = [this, family_index](vk::SurfaceKHR surface){
            return this->physdev.getSurfaceSupportKHR(family_index, surface);
        };

        if ((queue_families[family_index].queueFlags & flags) == flags &&
            std::all_of(surfaces.begin(), surfaces.end(), surface_supported)) {
            return family_index;
        }
    }

    return std::nullopt;
}
