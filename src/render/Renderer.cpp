#include "render/Renderer.h"

Renderer::Renderer(Display* display) {
    auto setup = display->setup();
    this->devices.reserve(setup.size());

    for (size_t i = 0; i < setup.size(); ++i) {
        this->devices.emplace_back(display, i, setup[i]);
    }
}

void Renderer::render() {
    for (auto& device : this->devices) {
        device.render();
    }
}

void Renderer::recreate(size_t gpu, size_t screen) {
    this->devices[gpu].recreate(screen);
}
