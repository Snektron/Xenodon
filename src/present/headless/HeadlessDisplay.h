#ifndef _XENODON_PRESENT_HEADLESS_HEADLESSDISPLAY_H
#define _XENODON_PRESENT_HEADLESS_HEADLESSDISPLAY_H

#include <vector>
#include <string_view>
#include <vulkan/vulkan.hpp>
#include "graphics/core/Instance.h"
#include "present/Display.h"
#include "present/Event.h"
#include "present/headless/HeadlessConfig.h"
#include "present/headless/HeadlessScreen.h"

struct Device;
struct Screen;

class HeadlessDisplay final: public Display {
    EventDispatcher& dispatcher;
    Instance instance;
    std::vector<HeadlessScreen> screens;
    const char* output;

public:
    HeadlessDisplay(EventDispatcher& dispatcher, const HeadlessConfig& config, const char* output);

    Setup setup() const override;
    Device& device(size_t gpu_index) override;
    Screen* screen(size_t gpu_index, size_t screen_index) override;
    void poll_events() override;

private:
    void save();
    void calculate_enclosing_rect();
};

#endif
