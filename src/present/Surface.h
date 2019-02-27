#ifndef _XENODON_PRESENT_SURFACE_H
#define _XENODON_PRESENT_SURFACE_H

#include <vulkan/vulkan.hpp>

struct Surface {
    virtual ~Surface() = 0;
    virtual vk::Rect2D area() = 0;
};

#endif
