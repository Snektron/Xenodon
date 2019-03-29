#ifndef _XENODON_PRESENT_XORG_XORGSCREEN_H
#define _XENODON_PRESENT_XORG_XORGSCREEN_H

#include <vulkan/vulkan.hpp>
#include <xcb/xcb.h>
#include "graphics/Device.h"
#include "graphics/Swapchain.h"
#include "present/Event.h"
#include "present/Screen.h"
#include "present/xorg/XorgWindow.h"

class XorgScreen final: public Screen {
    XorgWindow window;
    vk::UniqueSurfaceKHR surface;
    Device device;
    Swapchain swapchain;

public:
    XorgScreen(vk::Instance instance, EventDispatcher& dispatcher, vk::Extent2D extent);
    ~XorgScreen();

    void poll_events();

    uint32_t num_swap_images() const override;
    SwapImage swap_image(uint32_t index) const override;
    vk::Result present(Swapchain::PresentCallback f) override;

    vk::Rect2D region() const override;
    vk::AttachmentDescription color_attachment_descr() const override;

    friend class XorgDisplay;
};

#endif
