#include "backend/xorg/XorgDisplay.h"
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

    this->outputs.push_back(std::make_unique<XorgOutput>(this->instance, dispatcher, extent));
}

XorgDisplay::XorgDisplay(EventDispatcher& dispatcher, const XorgMultiGpuConfig& config):
    instance(REQUIRED_INSTANCE_EXTENSIONS) {

    this->outputs.reserve(config.outputs.size());
    for (const auto& output_conf : config.outputs) {
        this->outputs.push_back(std::make_unique<XorgOutput>(this->instance, dispatcher, output_conf));
    }
}

size_t XorgDisplay::num_render_devices() const {
    return this->outputs.size();
}

RenderDevice& XorgDisplay::render_device(size_t device_index) {
    return this->outputs[device_index]->render_device();
}

Output* XorgDisplay::output(size_t device_index, size_t output_index) {
    assert(output_index == 0);
    return this->outputs[device_index].get();
}

void XorgDisplay::swap_buffers() {
    for (auto& output : this->outputs) {
        output->swap_buffers();
    }
}

void XorgDisplay::poll_events() {
    for (auto& output : this->outputs) {
        output->poll_events();
    }
}
