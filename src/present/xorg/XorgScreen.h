#ifndef _XENODON_PRESENT_XORG_XORGSCREEN_H
#define _XENODON_PRESENT_XORG_XORGSCREEN_H

#include <vulkan/vulkan.hpp>
#include <xcb/xcb.h>
#include "graphics/Device.h"

class XorgWindow;

class XorgScreen {
    vk::UniqueSurfaceKHR surface;
    Device device;
    vk::Extent2D extent;

public:
    XorgScreen(vk::Instance instance, XorgWindow& window);
    vk::Extent2D size() const;
    void resize(uint16_t width, uint16_t height);

    friend class XorgDisplay;
};

#endif
