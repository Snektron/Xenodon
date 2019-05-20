#ifndef _XENODON_GRAPHICS_MEMORY_TEXTURE3D_H
#define _XENODON_GRAPHICS_MEMORY_TEXTURE3D_H

#include "graphics/core/Device.h"
#include "graphics/memory/Buffer.h"
#include "utility/Span.h"

class Texture3D {
    vk::Device dev;
    vk::Image image;
    vk::DeviceMemory mem;
    vk::ImageView image_view;

public:
    Texture3D(const Device& dev, vk::Format format, vk::Extent3D extent, vk::ImageUsageFlags flags);

    Texture3D(const Texture3D&) = delete;
    Texture3D& operator=(const Texture3D&) = delete;

    Texture3D(Texture3D&& other);
    Texture3D& operator=(Texture3D&& other);

    ~Texture3D();

    template <typename T>
    auto upload(Span<T> data, const vk::BufferImageCopy& region, vk::ImageLayout dst_layout);

    vk::Image get() const {
        return this->image;
    }

    vk::DeviceMemory memory() const {
        return this->mem;
    }

    vk::ImageView view() const {
        return this->image_view;
    }

    vk::Device device() const {
        return this->dev;
    }
};

#endif
