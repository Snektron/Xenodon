#ifndef _XENODON_INTERACTIVE_DISPLAY_H
#define _XENODON_INTERACTIVE_DISPLAY_H

#include <xcb/xcb.h>
#include <vulkan/vulkan.hpp>
#include "interactive/Window.h"
#include "interactive/Swapchain.h"
#include "render/Renderer.h"
#include "render/DeviceContext.h"

class Display {
    vk::Rect2D area;
    Window window;
    vk::UniqueSurfaceKHR surface;
    Swapchain swapchain;
    DeviceContext device_context;
    Renderer renderer;

public:
    void reconfigure(vk::Rect2D area);

    xcb_window_t xid() const {
        return window.xid;
    }
};

#endif
