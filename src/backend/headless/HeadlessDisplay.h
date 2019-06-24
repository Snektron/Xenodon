#ifndef _XENODON_BACKEND_HEADLESS_HEADLESSDISPLAY_H
#define _XENODON_BACKEND_HEADLESS_HEADLESSDISPLAY_H

#include <vector>
#include <string_view>
#include <filesystem>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "graphics/core/Instance.h"
#include "backend/Display.h"
#include "backend/Event.h"
#include "backend/headless/HeadlessConfig.h"
#include "backend/headless/HeadlessOutput.h"

struct Output;

class HeadlessDisplay final: public Display {
    Instance instance;
    std::vector<HeadlessOutput> outputs;
    std::string out_path;
    size_t frame;

public:
    HeadlessDisplay(const HeadlessConfig& config, std::string_view out_path);

    size_t num_render_devices() const override;
    const RenderDevice& render_device(size_t device_index) override;
    Output* output(size_t device_index, size_t output_index) override;
    void swap_buffers() override;
    void poll_events() override;

private:
    void save(const std::filesystem::path& path);
};

#endif
