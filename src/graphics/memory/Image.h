#ifndef _XENODON_GRAPHICS_MEMORY_IMAGE_H
#define _XENODON_GRAPHICS_MEMORY_IMAGE_H

#include "graphics/core/Device.h"

class Image {
    vk::UniqueImage image;
    vk::UniqueDeviceMemory mem;

public:
    constexpr const static vk::Format DEFAULT_FORMAT = vk::Format::eR8G8B8A8Unorm;
    constexpr const static vk::ImageUsageFlagBits DEFAULT_USAGE = vk::ImageUsageFlagBits::eSampled;

    Image(const Device& device, vk::Extent2D extent, vk::Format format = DEFAULT_FORMAT, vk::ImageUsageFlags usage = DEFAULT_USAGE);

    vk::Image get() const {
        return this->image.get();
    }

    vk::DeviceMemory memory() const {
        return this->mem.get();
    }
};

#endif
