#ifndef _XENODON_PRESENT_DISPLAY_H
#define _XENODON_PRESENT_DISPLAY_H

#include <vulkan/vulkan.hpp>
#include "utility/Span.h"

struct Display {
    virtual ~Display() = default;
    virtual vk::Extent2D size() const = 0;
    virtual void poll_events() = 0;
};

#endif
