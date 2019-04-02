#ifndef _XENODON_GRAPHICS_CORE_INSTANCE_H
#define _XENODON_GRAPHICS_CORE_INSTANCE_H

#include <vector>
#include <vulkan/vulkan.hpp>
#include "graphics/core/PhysicalDevice.h"
#include "utility/Span.h"

class Instance {
    vk::UniqueInstance instance;
    std::vector<PhysicalDevice> physdevs;

public:
    Instance(Span<const char* const> extensions = nullptr);

    vk::Instance get() const {
        return this->instance.get();
    }

    const vk::Instance* operator->() const {
        return &*this->instance;
    }

    const std::vector<PhysicalDevice>& physical_devices() {
        return this->physdevs;
    }
};

#endif
