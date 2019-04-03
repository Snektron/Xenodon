#include "render/Renderer.h"

Renderer::Renderer(Display* display) {
    size_t n = display->num_render_devices();
    this->devices.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        this->devices.push_back(std::make_unique<DeviceRenderer>(display, i));
    }
}

void Renderer::render() {
    for (auto& device : this->devices) {
        device->render();
    }
}

void Renderer::recreate(size_t device, size_t output) {
    this->devices[device]->recreate(output);
}
