#include "present/headless/HeadlessDisplay.h"
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
}

HeadlessDisplay::HeadlessDisplay(EventDispatcher& dispatcher, const HeadlessConfig& config):
    dispatcher(dispatcher),
    instance(vk::createInstanceUnique(INSTANCE_CREATE_INFO)) {
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
    this->dispatcher.dispatch_close_event();
}
