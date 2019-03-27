#include "present/headless/HeadlessDisplay.h"
#include <vector>
#include <lodepng.h>
#include "core/Logger.h"
#include "utility/enclosing_rect.h"
#include "version.h"

namespace {
    const auto INSTANCE_CREATE_INFO = vk::InstanceCreateInfo{
        {},
        &version::APP_INFO,
        0,
        nullptr,
        0,
        nullptr
    };

    constexpr const auto BLACK_PIXEL = 0xFF000000;
}

HeadlessDisplay::HeadlessDisplay(EventDispatcher& dispatcher, const HeadlessConfig& config):
    dispatcher(dispatcher),
    instance(vk::createInstanceUnique(INSTANCE_CREATE_INFO)) {

    auto gpus = this->instance->enumeratePhysicalDevices();
    this->screens.reserve(config.gpus.size());
    for (const auto gpu_config : config.gpus) {
        this->screens.emplace_back(gpus.at(gpu_config.vulkan_index), gpu_config.region);
    }
}

Setup HeadlessDisplay::setup() const {
    return Setup(this->screens.size(), 1);
}

Device& HeadlessDisplay::device_at(size_t gpu_index) {
    return this->screens[gpu_index].device;
}

Screen* HeadlessDisplay::screen_at(size_t gpu_index, size_t screen_index) {
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
    size_t stride = static_cast<size_t>(enclosing.extent.width);

    for (auto& screen : screens) {
        auto pixels = screen.download();

        size_t start_x = static_cast<size_t>(screen.render_region.offset.x - enclosing.offset.x);
        size_t start_y = static_cast<size_t>(screen.render_region.offset.y - enclosing.offset.y);
        size_t width = static_cast<size_t>(screen.render_region.extent.width);
        size_t height = static_cast<size_t>(screen.render_region.extent.height);

        for (size_t y = 0; y < height; ++y) {
            size_t yy = start_y + y;

            for (size_t x = 0; x < width; ++x) {
                size_t xx = start_x + x;

                image[xx + yy * stride] = pixels[x + y * width];
            }
        }
    }

    LOGGER.log("Compressing...");

    unsigned error = lodepng::encode(
        "out.png",
        reinterpret_cast<const unsigned char*>(image.data()),
        enclosing.extent.width,
        enclosing.extent.height
    );

    if (error) {
        LOGGER.log("Error saving output: {}", lodepng_error_text(error));
    } else {
        LOGGER.log("Saved output to 'out.png'", enclosing.extent.width, enclosing.extent.height);
    }
}
