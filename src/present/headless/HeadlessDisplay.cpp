#include "present/headless/HeadlessDisplay.h"
#include <vector>
#include <cassert>
#include <lodepng.h>
#include "core/Logger.h"
#include "utility/enclosing_rect.h"

namespace {
    constexpr const auto BLACK_PIXEL = 0xFF000000;
}

HeadlessDisplay::HeadlessDisplay(EventDispatcher& dispatcher, const HeadlessConfig& config, const char* out_path):
    dispatcher(dispatcher),
    instance(nullptr),
    out_path(out_path) {

    auto gpus = this->instance.physical_devices();
    this->outputs.reserve(config.gpus.size());
    for (const auto gpu_config : config.gpus) {
        this->outputs.emplace_back(gpus.at(gpu_config.vulkan_index), gpu_config.region);
    }
}

Setup HeadlessDisplay::setup() const {
    return Setup(this->outputs.size(), 1);
}

Device& HeadlessDisplay::device(size_t gpu_index) {
    return this->outputs[gpu_index].device;
}

Output* HeadlessDisplay::output(size_t gpu_index, size_t output_index) {
    // output index should always be 0 because there is always one output per render device.
    assert(output_index == 0);
    return &this->outputs[gpu_index];
}

void HeadlessDisplay::poll_events() {
    this->save();
    this->dispatcher.dispatch_close_event();
}

void HeadlessDisplay::save() {
    vk::Rect2D enclosing = enclosing_rect(this->outputs.begin(), this->outputs.end(), [](HeadlessOutput& output){
        return output.region();
    });

    LOGGER.log("Saving image...");
    LOGGER.log("Enclosing size: {}x{}", enclosing.extent.width, enclosing.extent.height);

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
        this->out_path,
        reinterpret_cast<const unsigned char*>(image.data()),
        enclosing.extent.width,
        enclosing.extent.height
    );

    if (error) {
        LOGGER.log("Error saving output: {}", lodepng_error_text(error));
    } else {
        LOGGER.log("Saved output to '{}'", this->out_path);
    }
}
