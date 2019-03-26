#ifndef _XENODON_PRESENT_HEADLESS_HEADLESSDISPLAY_H
#define _XENODON_PRESENT_HEADLESS_HEADLESSDISPLAY_H

#include <vulkan/vulkan.hpp>
#include "graphics/Device.h"
#include "present/Display.h"
#include "present/Event.h"
#include "present/headless/HeadlessConfig.h"

class HeadlessDisplay final: public Display {
    vk::UniqueInstance instance;
    EventDispatcher& dispatcher;

public:
    HeadlessDisplay(EventDispatcher& dispatcher, const HeadlessConfig& config);

    Setup setup() const override;
    Device& device_at(size_t gpu_index) const override;
    Screen* screen_at(size_t gpu_index, size_t screen_index) const override;
    void poll_events() override;
};

#endif
