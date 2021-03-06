#include "graphics/core/PhysicalDevice.h"
#include <algorithm>
#include <vector>
#include <cstring>
#include <fmt/format.h>
#include "core/Logger.h"

PhysicalDevice::PhysicalDevice(vk::PhysicalDevice physdev):
    physdev(physdev),
    props(this->physdev.getProperties()) {
}

bool PhysicalDevice::supports_extensions(Span<const char*> extensions) const {
    const auto supported_extensions = this->physdev.enumerateDeviceExtensionProperties();

    for (const char* const ext_name : extensions) {
        auto cmp_ext = [&ext_name](auto& properties) {
            return std::strcmp(ext_name, properties.extensionName) == 0;
        };

        if (std::none_of(supported_extensions.begin(), supported_extensions.end(), cmp_ext)) {
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

std::optional<uint32_t> PhysicalDevice::find_queue_family(vk::QueueFlags flags, Span<vk::SurfaceKHR> surfaces) const {
    auto queue_families = this->physdev.getQueueFamilyProperties();
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

std::optional<PhysicalDevice::QueueFamilyIndices> PhysicalDevice::find_queues(Span<vk::SurfaceKHR> surfaces) const {
    auto queue_families = this->physdev.getQueueFamilyProperties();
    uint32_t num_families = static_cast<uint32_t>(queue_families.size());

    auto graphics_index = std::optional<uint32_t>();
    auto compute_index = std::optional<uint32_t>();

    for (uint32_t family_index = 0; family_index < num_families; ++family_index) {
        const auto& props = queue_families[family_index];

        if (!graphics_index && (props.queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics) {
            bool surface_supported = std::all_of(
                surfaces.begin(),
                surfaces.end(),
                [this, family_index](vk::SurfaceKHR surface){
                    return this->physdev.getSurfaceSupportKHR(family_index, surface);
                }
            );

            if (surface_supported) {
                graphics_index = family_index;
            }
        } else if (!compute_index && (props.queueFlags & vk::QueueFlagBits::eCompute) == vk::QueueFlagBits::eCompute) {
            compute_index = family_index;
        }

        if (graphics_index && compute_index) {
            return QueueFamilyIndices{
                .graphics_family = graphics_index.value(),
                .compute_family = compute_index.value()
            };
        }
    }

    if (graphics_index) {
        const auto& props = queue_families[graphics_index.value()];
        if ((props.queueFlags & vk::QueueFlagBits::eCompute) == vk::QueueFlagBits::eCompute) {
            return QueueFamilyIndices{
                .graphics_family = graphics_index.value(),
                .compute_family = graphics_index.value()
            };
        }
    }

    return std::nullopt;
}

std::optional<PhysicalDevice::PlaneIndices> PhysicalDevice::find_display_plane(vk::DisplayKHR display) const {
    const auto plane_props = this->physdev.getDisplayPlanePropertiesKHR();

    for (uint32_t plane = 0; plane < plane_props.size(); ++plane) {
        if (plane_props[plane].currentDisplay != vk::DisplayKHR() && plane_props[plane].currentDisplay != display)
            continue;

        auto supported_displays = this->physdev.getDisplayPlaneSupportedDisplaysKHR(plane);
        for (uint32_t i = 0; i < supported_displays.size(); ++i) {
            if (supported_displays[i] == display) {
                return {{plane, plane_props[plane].currentStackIndex}};
            }
        }
    }

    return std::nullopt;
}
