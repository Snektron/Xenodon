#ifndef _XENODON_PRESENT_BARE_BAREDISPLAY_H
#define _XENODON_PRESENT_BARE_BAREDISPLAY_H

#include <vulkan/vulkan.hpp>
#include "present/Display.h"
#include "present/Event.h"

class BareDisplay: public Display {
    vk::Instance instance;
    EventDispatcher& dispatcher;

public:
    BareDisplay(vk::Instance instance, EventDispatcher& dispatcher);
    ~BareDisplay() override = default;
    void poll_events() override;
};

#endif
