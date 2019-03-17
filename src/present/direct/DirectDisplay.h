#ifndef _XENODON_PRESENT_DIRECT_DIRECTDISPLAY_H
#define _XENODON_PRESENT_DIRECT_DIRECTDISPLAY_H

#include <vector>
#include <vulkan/vulkan.hpp>
#include "present/Display.h"
#include "present/Event.h"
#include "present/direct/DisplayConfig.h"
#include "present/direct/DirectScreenGroup.h"

class DirectDisplay: public Display {
    std::vector<DirectScreenGroup> screen_groups;

public:
    DirectDisplay(vk::Instance instance, EventDispatcher& dispatcher, const DisplayConfig& display_config);
    ~DirectDisplay() override = default;

    vk::Extent2D size() const override;
    void poll_events() override;
};

#endif
