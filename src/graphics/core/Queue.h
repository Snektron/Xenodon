#ifndef _XENODON_GRAPHICS_CORE_QUEUE_H
#define _XENODON_GRAPHICS_CORE_QUEUE_H

#include <vulkan/vulkan.hpp>
#include "graphics/core/Device.h"

class Queue2 {
    vk::Queue queue;
    uint32_t family_index;
    uint32_t index;

public:
    Queue2(const Device2& device, uint32_t family_index, uint32_t index = 0):
        queue(device->getQueue(family_index, index)),
        family_index(family_index),
        index(index) {
    }

    vk::Queue get() const {
        return this->queue;
    }

    const vk::Queue* operator->() const {
        return &this->queue;
    }

    const uint32_t& queue_family_index() const {
        return this->family_index;
    }

    const uint32_t& queue_index() const {
        return this->index;
    }
};

#endif
