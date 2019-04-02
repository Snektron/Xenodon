#include "present/xorg/XorgDisplay.h"
#include <stdexcept>
#include <utility>
#include <array>
#include <cassert>

namespace {
    constexpr const std::array REQUIRED_INSTANCE_EXTENSIONS = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
    };
}

XorgDisplay::XorgDisplay(EventDispatcher& dispatcher, vk::Extent2D extent):
    instance(REQUIRED_INSTANCE_EXTENSIONS) {

    this->outputs.generate_in_place(1, [&](XorgOutput* ptr, [[maybe_unused]] size_t i){
        new (ptr) XorgOutput(this->instance, dispatcher, extent);
    });
}

XorgDisplay::XorgDisplay(EventDispatcher& dispatcher, const XorgMultiGpuConfig& config):
    instance(REQUIRED_INSTANCE_EXTENSIONS) {

    this->outputs.generate_in_place(config.outputs.size(), [&](XorgOutput* ptr, size_t i){
        new (ptr) XorgOutput(this->instance, dispatcher, config.outputs[i]);
    });
}

Setup XorgDisplay::setup() const {
    return Setup(this->outputs.size(), 1);
}

Device& XorgDisplay::device(size_t gpu_index) {
    return this->outputs[gpu_index].device;
}

Output* XorgDisplay::output(size_t gpu_index, size_t output_index) {
    /// output index should always be 0 because there is always one output per render device.
    assert(output_index == 0);
    return &this->outputs[gpu_index];
}

void XorgDisplay::poll_events() {
    for (auto& output : this->outputs) {
        output.poll_events();
    }
}
