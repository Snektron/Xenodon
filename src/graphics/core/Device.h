#ifndef _XENODON_GRAPHICS_CORE_DEVICE_H
#define _XENODON_GRAPHICS_CORE_DEVICE_H

#include <optional>
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include "graphics/core/PhysicalDevice.h"
#include "utility/Span.h"

class Device {
    vk::PhysicalDevice physdev;
    vk::UniqueDevice dev;

public:
    Device(const PhysicalDevice& physdev, Span<vk::DeviceQueueCreateInfo> queue_families, Span<const char* const> extensions = nullptr);
    Device(const PhysicalDevice& physdev, Span<uint32_t> queue_families, Span<const char* const> extensions = nullptr);

    std::optional<uint32_t> find_memory_type(uint32_t filter, vk::MemoryPropertyFlags flags) const;
    vk::DeviceMemory allocate(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags flags) const;
    vk::UniqueDeviceMemory allocate_unique(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags flags) const;

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
