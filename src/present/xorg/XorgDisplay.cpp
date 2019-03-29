#include "present/xorg/XorgDisplay.h"
#include <stdexcept>
#include <utility>
#include <array>
#include "version.h"

namespace {
    constexpr const std::array REQUIRED_INSTANCE_EXTENSIONS = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
    };

    const auto INSTANCE_CREATE_INFO = vk::InstanceCreateInfo{
        {},
        &version::APP_INFO,
        0,
        nullptr,
        REQUIRED_INSTANCE_EXTENSIONS.size(),
        REQUIRED_INSTANCE_EXTENSIONS.data()
    };
}

XorgDisplay::XorgDisplay(EventDispatcher& dispatcher, vk::Extent2D extent):
    instance(vk::createInstanceUnique(INSTANCE_CREATE_INFO)) {
    this->screens.generate_in_place(1, [&](XorgScreen* ptr, size_t i){
        new (ptr) XorgScreen(this->instance.get(), dispatcher, extent);
    });
}

XorgDisplay::XorgDisplay(EventDispatcher& dispatcher, const XorgMultiGpuConfig& config):
    instance(vk::createInstanceUnique(INSTANCE_CREATE_INFO)) {

    this->screens.generate_in_place(config.screens.size(), [&](XorgScreen* ptr, size_t i){
        new (ptr) XorgScreen(this->instance.get(), dispatcher, config.screens[i]);
    });
}

Setup XorgDisplay::setup() const {
    return Setup(this->screens.size(), 1);
}

Device& XorgDisplay::device_at(size_t gpu_index) {
    return this->screens[gpu_index].device;
}

Screen* XorgDisplay::screen_at(size_t gpu_index, size_t screen_index) {
    // screen_index should be 0.
    return &this->screens[gpu_index];
}

void XorgDisplay::poll_events() {
    for (auto& screen : this->screens) {
        screen.poll_events();
    }
}
