#ifndef _XENODON_GRAPHICS_CORE_DEVICE_H
#define _XENODON_GRAPHICS_CORE_DEVICE_H

#include <cstdint>
#include <vulkan/vulkan.hpp>
#include "graphics/core/PhysicalDevice.h"
#include "utility/Span.h"

class Device2 {
    vk::PhysicalDevice physdev;
    vk::UniqueDevice dev;

public:
    Device2(const PhysicalDevice& physdev, Span<vk::DeviceQueueCreateInfo> queue_families, Span<const char* const> extensions = nullptr);
    Device2(const PhysicalDevice& physdev, Span<uint32_t> queue_families, Span<const char* const> extensions = nullptr);

    vk::Device get() const {
        return this->dev.get();
    }

    const vk::Device* operator->() const {
        return &*this->dev;
    }

    vk::PhysicalDevice physical_device() const {
        return this->physdev;
    }
};

#endif
