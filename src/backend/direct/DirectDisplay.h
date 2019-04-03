#ifndef _XENODON_BACKEND_DIRECT_DIRECTDISPLAY_H
#define _XENODON_BACKEND_DIRECT_DIRECTDISPLAY_H

#include <vector>
#include <vulkan/vulkan.hpp>
#include "graphics/core/Instance.h"
#include "backend/Display.h"
#include "backend/Event.h"
#include "backend/direct/DirectConfig.h"
#include "backend/direct/ScreenGroup.h"
#include "backend/direct/input/LinuxInput.h"

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
