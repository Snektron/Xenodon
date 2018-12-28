#ifndef _XENODON_GRAPHICS_DEBUG_UTILITY_H
#define _XENODON_GRAPHICS_DEBUG_UTILITY_H

#include <vulkan/vulkan.hpp>

template <typename Os>
Os& operator<<(Os& os, vk::PhysicalDeviceType ty) {
    switch (ty) {
        case vk::PhysicalDeviceType::eOther:
            return os << "Other";
        case vk::PhysicalDeviceType::eIntegratedGpu:
            return os << "Integrated GPU";
        case vk::PhysicalDeviceType::eDiscreteGpu:
            return os << "Discrete GPU";
        case vk::PhysicalDeviceType::eVirtualGpu:
            return os << "Virtual GPU";
        case vk::PhysicalDeviceType::eCpu:
            return os << "CPU";
    }
}

#endif
