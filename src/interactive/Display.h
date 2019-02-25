#ifndef _XENODON_INTERACTIVE_DISPLAY_H
#define _XENODON_INTERACTIVE_DISPLAY_H

#include <vector>
#include <memory>
#include <cstddef>
#include <xcb/xcb.h>
#include <vulkan/vulkan.hpp>
#include "interactive/Window.h"
#include "interactive/Swapchain.h"
#include "interactive/SurfaceInfo.h"
#include "render/RenderWorker.h"
#include "render/DeviceContext.h"

class Display {
    struct FrameSync {
        vk::UniqueSemaphore image_available;
        vk::UniqueSemaphore render_finished;
        vk::UniqueFence fence;

        FrameSync(vk::Device device);
    };

    DeviceContext device_context;
    Window window;
    vk::UniqueSurfaceKHR surface;
    SurfaceInfo sinf;
    vk::Rect2D area;
    std::unique_ptr<RenderWorker> renderer;
    Swapchain swapchain;

    std::vector<FrameSync> sync_objects;
    size_t current_frame;

public:
    Display(DeviceContext&& device_context, Window&& window, vk::UniqueSurfaceKHR&& surface, vk::Rect2D area);
    ~Display();

    Display(Display&&) = delete;
    Display& operator=(Display&&) = delete;

    void reconfigure(vk::Rect2D area);
    void recreate_swapchain(vk::Extent2D window_extent);
    void present();

    xcb_window_t xid() const {
        return window.xid;
    }
};

#endif
