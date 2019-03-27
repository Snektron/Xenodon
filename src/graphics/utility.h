#ifndef _XENODON_GRAPHICS_UTILITY_H
#define _XENODON_GRAPHICS_UTILITY_H

#include <optional>
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include "utility/Span.h"

bool gpu_supports_extensions(vk::PhysicalDevice gpu, Span<const char* const> extensions);
bool gpu_supports_surface(vk::PhysicalDevice gpu, vk::SurfaceKHR surface);
std::optional<uint32_t> pick_graphics_queue(vk::PhysicalDevice gpu, Span<const vk::SurfaceKHR> surfaces);

#endif
