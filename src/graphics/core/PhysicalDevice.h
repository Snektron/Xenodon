#ifndef _XENODON_GRAPHICS_CORE_PHYSICALDEVICE_H
#define _XENODON_GRAPHICS_CORE_PHYSICALDEVICE_H

#include <optional>
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include "utility/Span.h"

class PhysicalDevice {
    vk::PhysicalDevice physdev;
    vk::PhysicalDeviceProperties props;

public:
    struct QueueFamilyIndices {
        uint32_t graphics_family, compute_family;
    };

    struct PlaneIndices {
        uint32_t plane_index, stack_index;
    };

    PhysicalDevice(vk::PhysicalDevice physdev);

    bool supports_extensions(Span<const char*> extensions) const;
    bool supports_surface(vk::SurfaceKHR surface) const;
    std::optional<uint32_t> find_queue_family(vk::QueueFlags flags, Span<vk::SurfaceKHR> surfaces = nullptr) const;
    std::optional<QueueFamilyIndices> find_queues(Span<vk::SurfaceKHR> surfaces = nullptr) const;
    std::optional<PlaneIndices> find_display_plane(vk::DisplayKHR display) const;

    vk::PhysicalDevice get() const {
        return this->physdev;
    }

    const vk::PhysicalDevice* operator->() const {
        return &this->physdev;
    }

    const vk::PhysicalDeviceProperties& properties() const {
        return this->props;
    }

    vk::PhysicalDeviceType type() const {
        return this->props.deviceType;
    }

    const char* name() const {
        return this->props.deviceName;
    }
};

#endif
