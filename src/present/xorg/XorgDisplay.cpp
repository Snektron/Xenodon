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
    instance(vk::createInstanceUnique(INSTANCE_CREATE_INFO)),
    screen(this->instance.get(), dispatcher, extent) {
}

Setup XorgDisplay::setup() const {
    return {1};
}

Device& XorgDisplay::device_at(size_t gpu_index) {
    // gpu_index should be 0.
    return this->screen.device;
}

Screen* XorgDisplay::screen_at(size_t gpu_index, size_t screen_index) {
    // gpu_index and screen_index should be 0.
    return &this->screen;
}

void XorgDisplay::poll_events() {
    this->screen.poll_events();
}
