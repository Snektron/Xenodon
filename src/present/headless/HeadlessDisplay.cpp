#include "present/headless/HeadlessDisplay.h"
#include <vector>
#include <lodepng.h>
#include "core/Logger.h"
#include "utility/enclosing_rect.h"

namespace {
    constexpr const auto BLACK_PIXEL = 0xFF000000;
}

HeadlessDisplay::HeadlessDisplay(EventDispatcher& dispatcher, const HeadlessConfig& config, const char* output):
    dispatcher(dispatcher),
    instance(nullptr),
    output(output) {

    auto gpus = this->instance.get().enumeratePhysicalDevices();
    this->screens.reserve(config.gpus.size());
    for (const auto gpu_config : config.gpus) {
        this->screens.emplace_back(gpus.at(gpu_config.vulkan_index), gpu_config.region);
    }
}

Setup HeadlessDisplay::setup() const {
    return Setup(this->screens.size(), 1);
}

Device& HeadlessDisplay::device(size_t gpu_index) {
    return this->screens[gpu_index].device;
}

Screen* HeadlessDisplay::screen(size_t gpu_index, [[maybe_unused]] size_t screen_index) {
    // screen index should always be 0
    return &this->screens[gpu_index];
}

void HeadlessDisplay::poll_events() {
    this->save();
    this->dispatcher.dispatch_close_event();
}

void HeadlessDisplay::save() {
    vk::Rect2D enclosing = enclosing_rect(this->screens.begin(), this->screens.end(), [](HeadlessScreen& screen){
        return screen.render_region;
    });

    LOGGER.log("Saving image...");
    LOGGER.log("Enclosing size: {}x{}", enclosing.extent.width, enclosing.extent.height);

    auto image = std::vector<Pixel>(enclosing.extent.width * enclosing.extent.height, BLACK_PIXEL);
    size_t stride = enclosing.extent.width;

    for (auto& screen : screens) {
        size_t start_x = static_cast<size_t>(screen.render_region.offset.x - enclosing.offset.x);
        size_t start_y = static_cast<size_t>(screen.render_region.offset.y - enclosing.offset.y);

        size_t offset = start_y * stride + start_x;
        screen.download(image.data() + offset, stride);
    }

    LOGGER.log("Compressing...");

    unsigned error = lodepng::encode(
        this->output,
        reinterpret_cast<const unsigned char*>(image.data()),
        enclosing.extent.width,
        enclosing.extent.height
    );

    if (error) {
        LOGGER.log("Error saving output: {}", lodepng_error_text(error));
    } else {
        LOGGER.log("Saved output to '{}'", this->output);
    }
}
