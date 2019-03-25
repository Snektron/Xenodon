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

XorgDisplay::XorgDisplay(EventDispatcher& dispatcher, uint16_t width, uint16_t height):
    instance(vk::createInstanceUnique(INSTANCE_CREATE_INFO)),
    window(dispatcher, width, height),
    screen(this->instance.get(), this->window, {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}) {
}

Setup XorgDisplay::setup() {
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
    this->window.poll_events(this->screen);
}
