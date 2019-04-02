#ifndef _XENODON_PRESENT_DIRECT_SCREENGROUP_H
#define _XENODON_PRESENT_DIRECT_SCREENGROUP_H

#include <vector>
#include <vulkan/vulkan.hpp>
#include "graphics/core/Instance.h"
#include "graphics/core/PhysicalDevice.h"
#include "graphics/Device.h"
#include "backend/direct/DirectOutput.h"
#include "backend/direct/DirectConfig.h"

class ScreenGroup {
    std::vector<vk::UniqueSurfaceKHR> surfaces;
    Device device;
    std::vector<DirectOutput> outputs;

public:
    ScreenGroup(Instance& instance, const PhysicalDevice& gpu, const std::vector<DirectConfig::Output>& outputs);
    ScreenGroup(ScreenGroup&&) = default;
    ScreenGroup& operator=(ScreenGroup&&) = default;
    ~ScreenGroup();

    friend class DirectDisplay;
};

#endif
