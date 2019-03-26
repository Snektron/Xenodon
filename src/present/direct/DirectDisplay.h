#ifndef _XENODON_PRESENT_DIRECT_DIRECTDISPLAY_H
#define _XENODON_PRESENT_DIRECT_DIRECTDISPLAY_H

#include <vector>
#include <vulkan/vulkan.hpp>
#include "present/Display.h"
#include "present/Event.h"
#include "present/direct/DirectConfig.h"
#include "present/direct/ScreenGroup.h"
#include "present/direct/input/LinuxInput.h"

class DirectDisplay final: public Display {
    vk::UniqueInstance instance;
    std::vector<ScreenGroup> screen_groups;
    LinuxInput input;

public:
    DirectDisplay(EventDispatcher& dispatcher, const DirectConfig& display_config);
    ~DirectDisplay() override = default;

    Setup setup() const override;
    Device& device_at(size_t gpu_index) override;
    Screen* screen_at(size_t gpu_index, size_t screen_index) override;
    void poll_events() override;
};

#endif
