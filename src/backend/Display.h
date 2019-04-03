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
    virtual Setup setup() const = 0;
    virtual Device& device(size_t gpu_index) = 0;
    virtual Output* output(size_t gpu_index, size_t output_index) = 0;

    // virtual size_t num_render_devices() const;
    // virtual const RenderDevice& render_device(size_t index);

    virtual void poll_events() = 0;
};

#endif
