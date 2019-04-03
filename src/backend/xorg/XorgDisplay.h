#ifndef _XENODON_BACKEND_XORG_XORGDISPLAY_H
#define _XENODON_BACKEND_XORG_XORGDISPLAY_H

#include <cstdint>
#include <vulkan/vulkan.hpp>
#include <xcb/xcb.h>
#include "graphics/core/Instance.h"
#include "backend/Display.h"
#include "backend/Event.h"
#include "backend/xorg/XorgOutput.h"
#include "backend/xorg/XorgMultiGpuConfig.h"
#include "utility/DynamicArray.h"

struct Device;
struct Output;

class XorgDisplay final: public Display {
    Instance instance;
    DynamicArray<XorgOutput> outputs;

public:
    XorgDisplay(EventDispatcher& dispatcher, vk::Extent2D extent);
    XorgDisplay(EventDispatcher& dispatcher, const XorgMultiGpuConfig& config);

    XorgDisplay(XorgDisplay&& other) = default;
    XorgDisplay& operator=(XorgDisplay&& other) = default;

    Setup setup() const override;
    Device& device(size_t gpu_index) override;
    Output* output(size_t gpu_index, size_t output_index) override;
    void poll_events() override;
};

#endif
