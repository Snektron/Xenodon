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

class Logger;

class XorgDisplay final: public Display {
    vk::UniqueInstance instance;
    XorgWindow window;
    XorgScreen screen;

public:
    XorgDisplay(EventDispatcher& dispatcher, uint16_t width, uint16_t height);

    XorgDisplay(XorgDisplay&& other) = default;
    XorgDisplay& operator=(XorgDisplay&& other) = default;

    Setup setup() const override;
    Device& device_at(size_t gpu_index) override;
    Screen* screen_at(size_t gpu_index, size_t screen_index) override;
    void poll_events() override;
};

#endif
