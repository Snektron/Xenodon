#ifndef _XENODON_GRAPHICS_BUFFER_H
#define _XENODON_GRAPHICS_BUFFER_H

#include <vulkan/vulkan.hpp>
#include "graphics/Device.h"

struct Buffer {
    vk::UniqueBuffer buf;
    vk::UniqueDeviceMemory mem;

    Buffer(Device& device, vk::DeviceSize size, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags memory_flags);

    vk::Buffer buffer() const {
        return this->buf.get();
    }

    vk::DeviceMemory memory() const {
        return this->mem.get();
    }
};

#endif
