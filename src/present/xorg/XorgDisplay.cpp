#include "present/xorg/XorgDisplay.h"
#include <stdexcept>
#include <utility>
#include <array>

namespace {
    constexpr const std::array REQUIRED_INSTANCE_EXTENSIONS = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
    };
}

XorgDisplay::XorgDisplay(EventDispatcher& dispatcher, vk::Extent2D extent):
    instance(REQUIRED_INSTANCE_EXTENSIONS) {

    this->screens.generate_in_place(1, [&](XorgScreen* ptr, [[maybe_unused]] size_t i){
        new (ptr) XorgScreen(this->instance, dispatcher, extent);
    });
}

XorgDisplay::XorgDisplay(EventDispatcher& dispatcher, const XorgMultiGpuConfig& config):
    instance(REQUIRED_INSTANCE_EXTENSIONS) {

    this->screens.generate_in_place(config.screens.size(), [&](XorgScreen* ptr, size_t i){
        new (ptr) XorgScreen(this->instance, dispatcher, config.screens[i]);
    });
}

Setup XorgDisplay::setup() const {
    return Setup(this->screens.size(), 1);
}

Device& XorgDisplay::device(size_t gpu_index) {
    return this->screens[gpu_index].device;
}

Screen* XorgDisplay::screen(size_t gpu_index, [[maybe_unused]] size_t screen_index) {
    // screen_index should be 0.
    return &this->screens[gpu_index];
}

void XorgDisplay::poll_events() {
    for (auto& screen : this->screens) {
        screen.poll_events();
    }
}
