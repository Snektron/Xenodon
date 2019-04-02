#ifndef _XENODON_PRESENT_DIRECT_DIRECTDISPLAY_H
#define _XENODON_PRESENT_DIRECT_DIRECTDISPLAY_H

#include <vector>
#include <vulkan/vulkan.hpp>
#include "graphics/core/Instance.h"
#include "present/Display.h"
#include "present/Event.h"
#include "present/direct/DirectConfig.h"
#include "present/direct/ScreenGroup.h"
#include "present/direct/input/LinuxInput.h"

struct Device;
struct Output;

class DirectDisplay final: public Display {
    Instance instance;
    std::vector<ScreenGroup> screen_groups;
    LinuxInput input;

public:
    DirectDisplay(EventDispatcher& dispatcher, const DirectConfig& display_config);
    ~DirectDisplay() override = default;

    Setup setup() const override;
    Device& device(size_t gpu_index) override;
    Output* output(size_t gpu_index, size_t output_index) override;
    void poll_events() override;
};

#endif
