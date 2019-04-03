#ifndef _XENODON_GRAPHICS_MEMORY_BUFFER_H
#define _XENODON_GRAPHICS_MEMORY_BUFFER_H

#include <vulkan/vulkan.hpp>
#include "graphics/core/Device.h"

struct Buffer {
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory mem;

    Buffer(const Device& device, vk::DeviceSize size, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags memory_flags);

    vk::Buffer get() const {
        return this->buffer.get();
    }

    vk::DeviceMemory memory() const {
        return this->mem.get();
    }
};

#endif
