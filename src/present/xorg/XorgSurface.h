#ifndef _XENODON_PRESENT_XORG_XORGSURFACE_H
#define _XENODON_PRESENT_XORG_XORGSURFACE_H

#include <vulkan/vulkan.hpp>
#include <xcb/xcb.h>
#include "graphics/Device.h"

class XorgWindow;

class XorgSurface {
    vk::UniqueSurfaceKHR surface;
    Device device;

public:
    XorgSurface(vk::Instance instance, XorgWindow& window);
};

#endif
