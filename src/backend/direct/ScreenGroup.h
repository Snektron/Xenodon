#ifndef _XENODON_BACKEND_DIRECT_SCREENGROUP_H
#define _XENODON_BACKEND_DIRECT_SCREENGROUP_H

#include <vector>
#include <vulkan/vulkan.hpp>
#include "graphics/core/Instance.h"
#include "graphics/core/PhysicalDevice.h"
#include "graphics/core/Device.h"
#include "backend/RenderDevice.h"
#include "backend/direct/DirectOutput.h"
#include "backend/direct/DirectConfig.h"

class ScreenGroup {
    std::vector<vk::UniqueSurfaceKHR> surfaces;
    RenderDevice rendev;
    std::vector<DirectOutput> outputs;

public:
    ScreenGroup(Instance& instance, const PhysicalDevice& physdev, const std::vector<DirectConfig::Output>& outputs);
    ~ScreenGroup();

    void swap_buffers();
    void log() const;

    RenderDevice& render_device() {
        return this->rendev;
    }

    DirectOutput& output(size_t output_index) {
        return this->outputs[output_index];
    }
};

#endif
