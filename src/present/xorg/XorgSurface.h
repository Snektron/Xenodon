#ifndef _XENODON_PRESENT_XORG_XORGSURFACE_H
#define _XENODON_PRESENT_XORG_XORGSURFACE_H

#include <vulkan/vulkan.hpp>
#include <xcb/xcb.h>
#include "present/Surface.h"
#include "render/Device.h"

class XorgSurface {
    vk::UniqueSurfaceKHR surface;
    Device device;

public:
    XorgSurface() = default;
    XorgSurface(vk::Instance instance, xcb_connection_t* connection, xcb_window_t window);
};

#endif
