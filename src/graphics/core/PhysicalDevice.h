#ifndef _XENODON_GRAPHICS_CORE_PHYSICALDEVICE_H
#define _XENODON_GRAPHICS_CORE_PHYSICALDEVICE_H

#include <optional>
#include <utility>
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include "utility/Span.h"

class PhysicalDevice {
    vk::PhysicalDevice physdev;
    vk::PhysicalDeviceProperties props;

public:
    PhysicalDevice(vk::PhysicalDevice physdev);

    bool supports_extensions(Span<const char* const> extensions) const;
    bool supports_surface(vk::SurfaceKHR surface) const;
    std::optional<uint32_t> find_queue_family(vk::QueueFlags flags, Span<const vk::SurfaceKHR> surfaces = nullptr) const;
    std::optional<std::pair<uint32_t, uint32_t>> find_display_plane(vk::DisplayKHR display) const;

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
