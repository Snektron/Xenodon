#ifndef _XENODON_GRAPHICS_RENDERTARGET_H
#define _XENODON_GRAPHICS_RENDERTARGET_H

#include <vulkan/vulkan.hpp>
#include "graphics/Device.h"

struct RenderTarget {
    vk::UniqueImage image;
    vk::UniqueDeviceMemory memory;
    vk::UniqueImageView view;

    RenderTarget(Device& device, vk::Format format, vk::Extent2D extent);
};

#endif
