#ifndef _XENODON_PRESENT_XORG_XORGSCREEN_H
#define _XENODON_PRESENT_XORG_XORGSCREEN_H

#include <vulkan/vulkan.hpp>
#include <xcb/xcb.h>
#include "graphics/core/Instance.h"
#include "graphics/Device.h"
#include "graphics/Swapchain.h"
#include "backend/Event.h"
#include "backend/Output.h"
#include "backend/xorg/Window.h"
#include "backend/xorg/XorgMultiGpuConfig.h"

class XorgOutput final: public Output {
    Window window;
    vk::UniqueSurfaceKHR surface;
    Device device;
    Swapchain swapchain;

public:
    XorgOutput(Instance& instance, EventDispatcher& dispatcher, vk::Extent2D extent);
    XorgOutput(Instance& instance, EventDispatcher& dispatcher, const XorgMultiGpuConfig::Output& config);

    ~XorgOutput();

    XorgOutput(XorgOutput&& other) = delete;
    XorgOutput& operator=(XorgOutput&& other) = delete;

    void poll_events();

    uint32_t num_swap_images() const override;
    SwapImage swap_image(uint32_t index) const override;
    vk::Result present(Swapchain::PresentCallback f) override;

    vk::Rect2D region() const override;
    vk::AttachmentDescription color_attachment_descr() const override;

private:
    void log() const;

    friend class XorgDisplay;
};

#endif
