#ifndef _XENODON_PRESENT_DIRECT_DIRECTDISPLAY_H
#define _XENODON_PRESENT_DIRECT_DIRECTDISPLAY_H

#include <vector>
#include <vulkan/vulkan.hpp>
#include "present/Display.h"
#include "present/direct/DisplayConfig.h"
#include "present/direct/ScreenGroup.h"

class DirectDisplay final: public Display {
    std::vector<ScreenGroup> screen_groups;

public:
    DirectDisplay(vk::Instance instance, const DisplayConfig& display_config);
    ~DirectDisplay() override = default;

    Setup setup() override;
    Device& device_at(size_t gpu_index) override;
    Screen* screen_at(size_t gpu_index, size_t screen_index) override;
    void poll_events() override;
    void swap_buffers() override;
};

#endif
