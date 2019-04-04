#ifndef _XENODON_GRAPHICS_MEMORY_IMAGE_H
#define _XENODON_GRAPHICS_MEMORY_IMAGE_H

#include <vulkan/vulkan.hpp>
#include "graphics/core/Device.h"

class Image {
    vk::Device device;
    vk::Image image;
    vk::DeviceMemory mem;

public:
    constexpr const static vk::Format DEFAULT_FORMAT = vk::Format::eR8G8B8A8Unorm;
    constexpr const static vk::ImageUsageFlagBits DEFAULT_USAGE = vk::ImageUsageFlagBits::eSampled;

    Image(const Device& device, vk::Extent2D extent, vk::Format format = DEFAULT_FORMAT, vk::ImageUsageFlags usage = DEFAULT_USAGE);

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    Image(Image&& other);
    Image& operator=(Image&& other);

    ~Image();

    vk::Image get() const {
        return this->image;
    }

    vk::DeviceMemory memory() const {
        return this->mem;
    }
};

#endif
