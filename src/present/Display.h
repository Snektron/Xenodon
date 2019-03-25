#ifndef _XENODON_PRESENT_DISPLAY_H
#define _XENODON_PRESENT_DISPLAY_H

#include <vector>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "utility/Span.h"
#include "graphics/Device.h"
#include "present/Screen.h"

using Setup = std::vector<size_t>;

struct Display {
    virtual ~Display() = default;
    virtual Setup setup() = 0;
    virtual Device& device_at(size_t gpu_index) = 0;
    virtual Screen* screen_at(size_t gpu_index, size_t screen_index) = 0;
    virtual void poll_events() = 0;
};

#endif
