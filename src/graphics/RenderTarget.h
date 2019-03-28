#ifndef _XENODON_GRAPHICS_RENDERTARGET_H
#define _XENODON_GRAPHICS_RENDERTARGET_H

#include <vulkan/vulkan.hpp>
#include "graphics/Device.h"
#include "graphics/Image.h"

class RenderTarget {
    Image img;
    vk::UniqueImageView img_view;

public:
    const static vk::ImageUsageFlags USAGE_FLAGS;

    RenderTarget(Device& device, vk::Extent2D extent, vk::Format format = vk::Format::eR8G8B8A8Unorm);

    vk::Image image() const {
        return this->img.image();
    }

    vk::ImageView view() const {
        return this->img_view.get();
    }

    vk::DeviceMemory memory() const {
        return this->img.memory();
    }
};

#endif
