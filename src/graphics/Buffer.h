#ifndef _XENODON_GRAPHICS_BUFFER_H
#define _XENODON_GRAPHICS_BUFFER_H

#include <vulkan/vulkan.hpp>
#include "graphics/Device.h"

struct Buffer {
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;

    Buffer(Device& device, vk::DeviceSize size, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags memory_flags);
};

#endif
