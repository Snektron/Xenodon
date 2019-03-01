#ifndef _XENODON_PRESENT_DIRECT_DIRECTDISPLAY_H
#define _XENODON_PRESENT_DIRECT_DIRECTDISPLAY_H

#include <vulkan/vulkan.hpp>
#include "present/Display.h"
#include "present/Event.h"

class DirectDisplay: public Display {
public:
    DirectDisplay(vk::Instance instance, EventDispatcher& dispatcher);
    ~DirectDisplay() override = default;

    vk::Extent2D size() const override;
    void poll_events() override;
};

#endif
