#ifndef _XENODON_BACKEND_XORG_XORGOUTPUT_H
#define _XENODON_BACKEND_XORG_XORGOUTPUT_H

#include <vulkan/vulkan.hpp>
#include <xcb/xcb.h>
#include "graphics/core/Instance.h"
#include "graphics/core/Device.h"
#include "graphics/core/Swapchain.h"
#include "backend/Event.h"
#include "backend/Output.h"
#include "backend/RenderDevice.h"
#include "backend/xorg/Window.h"
#include "backend/xorg/XorgMultiGpuConfig.h"

class XorgOutput final: public Output {
    Window window;
    vk::UniqueSurfaceKHR surface;
    RenderDevice rendev;
    Swapchain swapchain;

public:
    XorgOutput(Instance& instance, EventDispatcher& dispatcher, vk::Extent2D extent);
    XorgOutput(Instance& instance, EventDispatcher& dispatcher, const XorgMultiGpuConfig::Output& config);

    ~XorgOutput();

    XorgOutput(XorgOutput&& other) = delete;
    XorgOutput& operator=(XorgOutput&& other) = delete;

    void swap_buffers();
    void poll_events();

    uint32_t num_swap_images() const override;
    uint32_t current_swap_index() const override;
    SwapImage swap_image(uint32_t index) override;
    vk::Rect2D region() const override;
    vk::AttachmentDescription color_attachment_descr() const override;

    RenderDevice& render_device() {
        return this->rendev;
    }

private:
    void log() const;
};

#endif
