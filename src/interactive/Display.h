#ifndef _XENODON_INTERACTIVE_DISPLAY_H
#define _XENODON_INTERACTIVE_DISPLAY_H

#include <vector>
#include <xcb/xcb.h>
#include <vulkan/vulkan.hpp>
#include "interactive/Window.h"
#include "interactive/Swapchain.h"
#include "render/Renderer.h"
#include "render/DeviceContext.h"

class Display {
    vk::Rect2D area;
    DeviceContext device_context;
    Window window;
    vk::UniqueSurfaceKHR surface;
    Swapchain swapchain;
    Renderer renderer;
    std::vector<vk::Framebuffer> framebuffers;
    std::vector<vk::CommandBuffer> command_buffers; 

public:
    Display(vk::Instance instance, Window window, vk::Rect2D area);
    void reconfigure(vk::Rect2D area);

    xcb_window_t xid() const {
        return window.xid;
    }
};

#endif
