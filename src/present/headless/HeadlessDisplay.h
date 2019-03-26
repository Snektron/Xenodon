#ifndef _XENODON_PRESENT_HEADLESS_HEADLESSDISPLAY_H
#define _XENODON_PRESENT_HEADLESS_HEADLESSDISPLAY_H

#include <vulkan/vulkan.hpp>
#include "graphics/Device.h"
#include "present/Display.h"
#include "present/Event.h"
#include "present/headless/HeadlessConfig.h"

class HeadlessDisplay final: public Display {
    EventDispatcher& dispatcher;
    vk::UniqueInstance instance;

public:
    HeadlessDisplay(EventDispatcher& dispatcher, const HeadlessConfig& config);

    Setup setup() const override;
    Device& device_at(size_t gpu_index) override;
    Screen* screen_at(size_t gpu_index, size_t screen_index) override;
    void poll_events() override;
};

#endif
