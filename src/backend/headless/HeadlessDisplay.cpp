#include "backend/headless/HeadlessDisplay.h"
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
    // TODO: make sure every frame is rendered
    for (const auto& output : this->outputs) {
        output.synchronize();
    }

    this->save();
}

void HeadlessDisplay::poll_events() {
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
