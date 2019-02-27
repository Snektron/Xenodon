#ifndef _XENODON_PRESENT_DISPLAY_H
#define _XENODON_PRESENT_DISPLAY_H

#include <vulkan/vulkan.hpp>
#include "utility/Span.h"

struct Surface {
    virtual vk::Rect2D area() = 0;
    virtual ~Surface() = 0;
};

struct Display {
    virtual vk::Extent2D size() = 0;
    virtual void poll_events() = 0;
    // virtual Span<Surface> surfaces() = 0;
    virtual ~Display() = default;
};

#endif
