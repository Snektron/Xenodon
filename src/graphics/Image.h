#ifndef _XENODON_GRAPHICS_IMAGE_H
#define _XENODON_GRAPHICS_IMAGE_H

#include <vulkan/vulkan.hpp>
#include "graphics/Device.h"

class Image {
    vk::UniqueImage img;
    vk::UniqueDeviceMemory mem;

public:
    constexpr const static vk::Format DEFAULT_FORMAT = vk::Format::eR8G8B8A8Unorm;
    const static vk::ImageUsageFlags DEFAULT_USAGE;

    Image(Device& device, vk::Extent2D extent, vk::Format format = DEFAULT_FORMAT, vk::ImageUsageFlags usage = DEFAULT_USAGE);

    vk::Image image() const {
        return this->img.get();
    }

    vk::DeviceMemory memory() const {
        return this->mem.get();
    }
};

#endif
