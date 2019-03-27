#ifndef _XENODON_GRAPHICS_DEVICE_CPP
#define _XENODON_GRAPHICS_DEVICE_CPP

#include <limits>
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include "utility/Span.h"

struct Queue {
    vk::Queue queue;
    uint32_t family_index;
    vk::UniqueCommandPool command_pool;

    Queue(vk::Device device, uint32_t family_index);

    static Queue invalid();

    bool is_valid() const;
};

struct Device {
    vk::PhysicalDevice physical;
    vk::UniqueDevice logical;

    Queue graphics;

    Device(vk::PhysicalDevice physical, Span<const char* const> extensions, uint32_t graphics_queue);
};

#endif
