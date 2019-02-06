#ifndef _XENODON_RENDER_DEVICECONTEXT_H
#define _XENODON_RENDER_DEVICECONTEXT_H

#include <limits>
#include <cstdint>
#include <vulkan/vulkan.hpp>

struct Queue {
    vk::Queue queue;
    uint32_t family_index;
    uint32_t queue_index;

    Queue(vk::Device device, uint32_t family_index, uint32_t queue_index = 0):
        queue(device.getQueue(family_index, queue_index)),
        family_index(family_index),
        queue_index(queue_index) {
    }

    static Queue invalid() {
        return {
            nullptr,
            std::numeric_limits<uint32_t>::max(),
            std::numeric_limits<uint32_t>::max()
        };
    }

    bool valid() const {
        return this->queue != vk::Queue(nullptr);
    }
};

struct DeviceContext {
    vk::PhysicalDevice physical_device;
    vk::Device device;

    Queue graphics;
    Queue present;

    vk::UniqueCommandPool graphics_command_pool;
};

#endif
