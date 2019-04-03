#ifndef _XENODON_BACKEND_XORG_XORGDISPLAY_H
#define _XENODON_BACKEND_XORG_XORGDISPLAY_H

#include <memory>
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include <xcb/xcb.h>
#include "graphics/core/Instance.h"
#include "backend/Display.h"
#include "backend/RenderDevice.h"
#include "backend/Event.h"
#include "backend/xorg/XorgOutput.h"
#include "backend/xorg/XorgMultiGpuConfig.h"

struct Device;
struct Output;

class XorgDisplay final: public Display {
    Instance instance;
    std::vector<std::unique_ptr<XorgOutput>> outputs;

public:
    XorgDisplay(EventDispatcher& dispatcher, vk::Extent2D extent);
    XorgDisplay(EventDispatcher& dispatcher, const XorgMultiGpuConfig& config);

    size_t num_render_devices() const override;
    RenderDevice& render_device(size_t device_index) override;
    Output* output(size_t device_index, size_t output_index) override;
    void swap_buffers() override;
    void poll_events() override;
};

#endif
