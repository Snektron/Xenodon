#ifndef _XENODON_PRESENT_DIRECT_SCREENGROUP_H
#define _XENODON_PRESENT_DIRECT_SCREENGROUP_H

#include <vector>
#include <vulkan/vulkan.hpp>
#include "graphics/Device.h"
#include "present/direct/DirectScreen.h"
#include "present/direct/DirectConfig.h"

class ScreenGroup {
    std::vector<vk::UniqueSurfaceKHR> surfaces;
    Device device;
    std::vector<DirectScreen> screens;

public:
    ScreenGroup(vk::Instance instance, vk::PhysicalDevice gpu, const std::vector<DirectConfig::Screen>& screens);
    ScreenGroup(ScreenGroup&&) = default;
    ScreenGroup& operator=(ScreenGroup&&) = default;
    ~ScreenGroup();
    void swap_buffers();

    friend class DirectDisplay;
};

#endif