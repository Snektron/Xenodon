#ifndef _XENODON_BACKEND_DISPLAY_H
#define _XENODON_BACKEND_DISPLAY_H

#include <vector>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "graphics/Device.h"
#include "backend/Output.h"
#include "backend/RenderDevice.h"
#include "utility/Span.h"

using Setup = std::vector<size_t>;

struct Display {
    virtual ~Display() = default;

    virtual size_t num_render_devices() const = 0;
    virtual const RenderDevice& render_device(size_t device_index) = 0;
    virtual Output* output(size_t device_index, size_t output_index) = 0;
    virtual void swap_buffers() = 0;
    virtual void poll_events() = 0;
};

#endif
