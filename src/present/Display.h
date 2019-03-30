#ifndef _XENODON_PRESENT_DISPLAY_H
#define _XENODON_PRESENT_DISPLAY_H

#include <vector>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "graphics/Device.h"
#include "present/Screen.h"

using Setup = std::vector<size_t>;

struct Display {
    virtual ~Display() = default;
    virtual Setup setup() const = 0;
    virtual Device& device(size_t gpu_index) = 0;
    virtual Screen* screen(size_t gpu_index, size_t screen_index) = 0;
    virtual void poll_events() = 0;
};

#endif
