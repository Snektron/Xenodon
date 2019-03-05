#ifndef _XENODON_PRESENT_XORG_XORGSCREEN_H
#define _XENODON_PRESENT_XORG_XORGSCREEN_H

#include <vulkan/vulkan.hpp>
#include <xcb/xcb.h>
#include "graphics/Device.h"
#include "graphics/Swapchain.h"

class XorgWindow;

class XorgScreen {
    vk::UniqueSurfaceKHR surface;
    Device device;
    Swapchain swapchain;

public:
    XorgScreen(vk::Instance instance, XorgWindow& window, vk::Extent2D window_extent);
    ~XorgScreen();

    vk::Extent2D size() const;
    void resize(vk::Extent2D window_extent);
    void swap_buffers();
};

#endif
