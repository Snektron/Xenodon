#ifndef _XENODON_PRESENT_XORG_XORGDISPLAY_H
#define _XENODON_PRESENT_XORG_XORGDISPLAY_H

#include <array>
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include <xcb/xcb.h>
#include "present/Display.h"
#include "present/Event.h"
#include "present/xorg/XorgScreen.h"
#include "present/xorg/XorgWindow.h"

class XorgDisplay: public Display {
    XorgWindow window;
    XorgScreen screen;

public:
    XorgDisplay(vk::Instance instance, EventDispatcher& dispatcher, uint16_t width, uint16_t height);

    XorgDisplay(const XorgDisplay&) = delete;
    XorgDisplay& operator=(const XorgDisplay&) = delete;
    
    XorgDisplay(XorgDisplay&& other) = default;
    XorgDisplay& operator=(XorgDisplay&& other) = default;
    
    vk::Extent2D size() const override;
    void poll_events() override;
    void swap_buffers() override;
};

#endif
