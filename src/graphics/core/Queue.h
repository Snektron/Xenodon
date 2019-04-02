#ifndef _XENODON_GRAPHICS_CORE_QUEUE_H
#define _XENODON_GRAPHICS_CORE_QUEUE_H

#include <vulkan/vulkan.hpp>
#include "graphics/core/Device.h"

class Queue {
    vk::Queue queue;
    uint32_t family_index;
    uint32_t index;

public:
    Queue(const Device2& device, uint32_t family_index, uint32_t index = 0);

    vk::Queue get() const {
        return this->queue;
    }

    uint32_t queue_family_index() const {
        return this->family_index;
    }

    uint32_t queue_index() const {
        return this->index;
    }
};

#endif
