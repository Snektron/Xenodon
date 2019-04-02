#ifndef _XENODON_PRESENT_HEADLESS_HEADLESSDISPLAY_H
#define _XENODON_PRESENT_HEADLESS_HEADLESSDISPLAY_H

#include <vector>
#include <string_view>
#include <vulkan/vulkan.hpp>
#include "graphics/core/Instance.h"
#include "present/Display.h"
#include "present/Event.h"
#include "present/headless/HeadlessConfig.h"
#include "present/headless/HeadlessOutput.h"

struct Device;
struct Output;

class HeadlessDisplay final: public Display {
    EventDispatcher& dispatcher;
    Instance instance;
    std::vector<HeadlessOutput> outputs;
    const char* out_path;

public:
    HeadlessDisplay(EventDispatcher& dispatcher, const HeadlessConfig& config, const char* out_path);

    Setup setup() const override;
    Device& device(size_t gpu_index) override;
    Output* output(size_t gpu_index, size_t output_index) override;
    void poll_events() override;

private:
    void save();
};

#endif
