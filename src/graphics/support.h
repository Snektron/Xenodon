#ifndef _XENODON_GRAPHICS_SUPPORT_H
#define _XENODON_GRAPHICS_SUPPORT_H

#include <vulkan/vulkan.hpp>
#include "utility/Span.h"

bool check_extension_support(vk::PhysicalDevice gpu, Span<const char* const> extensions);
bool check_surface_support(vk::PhysicalDevice gpu, vk::SurfaceKHR surface);

#endif
