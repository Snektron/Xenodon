#ifndef _XENODON_BACKEND_DIRECT_DIRECTDISPLAY_H
#define _XENODON_BACKEND_DIRECT_DIRECTDISPLAY_H

#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "graphics/core/Instance.h"
#include "backend/Display.h"
#include "backend/Event.h"
#include "backend/direct/DirectConfig.h"
#include "backend/direct/ScreenGroup.h"
#include "backend/direct/input/LinuxInput.h"

struct Output;

class DirectDisplay final: public Display {
    Instance instance;
    std::vector<std::unique_ptr<ScreenGroup>> screen_groups;
    LinuxInput input;

public:
    DirectDisplay(EventDispatcher& dispatcher, const DirectConfig& display_config);
    ~DirectDisplay() override = default;

    size_t num_render_devices() const override;
    const RenderDevice& render_device(size_t device_index) override;
    Output* output(size_t device_index, size_t output_index) override;
    void swap_buffers() override;
    void poll_events() override;
};

#endif
