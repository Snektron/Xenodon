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

}

Device& HeadlessDisplay::device_at(size_t gpu_index) const {

}

Screen* HeadlessDisplay::screen_at(size_t gpu_index, size_t screen_index) const {

}

void HeadlessDisplay::poll_events() {
    this->dispatcher.dispatch_close_event();
}
