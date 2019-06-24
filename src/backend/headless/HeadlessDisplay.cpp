#include "backend/headless/HeadlessDisplay.h"
#include <vector>
#include <cassert>
#include <fmt/format.h>
#include <lodepng.h>
#include "core/Logger.h"
#include "core/Error.h"
#include "utility/rect_union.h"

namespace {
    constexpr const auto BLACK_PIXEL = 0xFF000000;
}

HeadlessDisplay::HeadlessDisplay(const HeadlessConfig& config, std::string_view out_path):
    instance(nullptr),
    out_path(out_path),
    frame(0) {

    auto gpus = this->instance.physical_devices();
    this->outputs.reserve(config.gpus.size());
    for (const auto gpu_config : config.gpus) {
        this->outputs.emplace_back(gpus.at(gpu_config.vulkan_index), gpu_config.region);
    }
}

size_t HeadlessDisplay::num_render_devices() const {
    return this->outputs.size();
}

const RenderDevice& HeadlessDisplay::render_device(size_t device_index) {
    return this->outputs[device_index].render_device();
}

Output* HeadlessDisplay::output(size_t device_index, size_t output_index) {
    assert(output_index == 0);
    return &this->outputs[device_index];
}

void HeadlessDisplay::swap_buffers() {
    for (const auto& output : this->outputs) {
        output.synchronize();
    }

    if (!this->out_path.empty()) {
        try {
            auto path = fmt::format(this->out_path, this->frame);
            this->save(path);
        } catch (const fmt::format_error& e) {
            throw Error("Failed to format output filename: {}", e.what());
        }
    }

    ++this->frame;
}

void HeadlessDisplay::poll_events() {
}

void HeadlessDisplay::save(const std::filesystem::path& path) {
    vk::Rect2D enclosing = rect_union(this->outputs.begin(), this->outputs.end(), [](HeadlessOutput& output){
        return output.region();
    });

    LOGGER.log("Saving frame {}...", this->frame);

    auto image = std::vector<Pixel>(enclosing.extent.width * enclosing.extent.height, BLACK_PIXEL);
    size_t stride = enclosing.extent.width;

    for (auto& output : this->outputs) {
        vk::Rect2D region = output.region();
        size_t start_x = static_cast<size_t>(region.offset.x - enclosing.offset.x);
        size_t start_y = static_cast<size_t>(region.offset.y - enclosing.offset.y);

        size_t offset = start_y * stride + start_x;
        output.download(image.data() + offset, stride);
    }

    LOGGER.log("Compressing...");

    unsigned error = lodepng::encode(
        path.c_str(),
        reinterpret_cast<const unsigned char*>(image.data()),
        enclosing.extent.width,
        enclosing.extent.height
    );

    if (error) {
        LOGGER.log("Error saving output: {}", lodepng_error_text(error));
    } else {
        LOGGER.log("Saved output to '{}'", path.native());
    }
}
