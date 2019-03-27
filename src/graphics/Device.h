#ifndef _XENODON_GRAPHICS_DEVICE_CPP
#define _XENODON_GRAPHICS_DEVICE_CPP

#include <limits>
#include <cstdint>
#include <vulkan/vulkan.hpp>

struct Queue {
    vk::Queue queue;
    uint32_t family_index;

    Queue(vk::Device device, uint32_t family_index);
    Queue(std::nullptr_t);

    static Queue invalid();

    bool is_valid() const;
};

struct Device {
    vk::PhysicalDevice physical;
    vk::UniqueDevice logical;

    Queue graphics;

    vk::UniqueCommandPool graphics_command_pool;

    Device(vk::PhysicalDevice physical, vk::ArrayProxy<const char* const> extensions, uint32_t graphics_queue);
};

#endif
