#ifndef _XENODON_PRESENT_XORG_XORGDISPLAY_H
#define _XENODON_PRESENT_XORG_XORGDISPLAY_H

#include <cstdint>
#include <vulkan/vulkan.hpp>
#include <xcb/xcb.h>
#include "present/Display.h"
#include "present/Event.h"
#include "present/xorg/XorgScreen.h"
#include "present/xorg/XorgMultiGpuConfig.h"
#include "utility/DynamicArray.h"

class Logger;

class XorgDisplay final: public Display {
    vk::UniqueInstance instance;
    DynamicArray<XorgScreen> screens;

public:
    XorgDisplay(EventDispatcher& dispatcher, vk::Extent2D extent);
    XorgDisplay(EventDispatcher& dispatcher, const XorgMultiGpuConfig& config);

    XorgDisplay(XorgDisplay&& other) = default;
    XorgDisplay& operator=(XorgDisplay&& other) = default;

    Setup setup() const override;
    Device& device_at(size_t gpu_index) override;
    Screen* screen_at(size_t gpu_index, size_t screen_index) override;
    void poll_events() override;
};

#endif
