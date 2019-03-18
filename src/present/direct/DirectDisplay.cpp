#include "present/direct/DirectDisplay.h"

DirectDisplay::DirectDisplay(vk::Instance instance, EventDispatcher& dispatcher, const DirectConfig& display_config):
    input(dispatcher, display_config.input) {
    this->screen_groups.reserve(display_config.gpus.size());

    auto gpus = instance.enumeratePhysicalDevices();
    for (const auto& device : display_config.gpus) {
        if (device.vulkan_index >= gpus.size()) {
            throw std::runtime_error("Vulkan screen index out of range");
        }

        this->screen_groups.emplace_back(instance, gpus[device.vulkan_index], device.screens);
    }
}

Setup DirectDisplay::setup() {
    auto setup = Setup();
    setup.resize(this->screen_groups.size());

    for (size_t i = 0; i < this->screen_groups.size(); ++i) {
        setup[i] = this->screen_groups[i].screens.size();
    }

    return setup;
}

Device& DirectDisplay::device_at(size_t gpu_index) {
    return this->screen_groups[gpu_index].device;
}

Screen* DirectDisplay::screen_at(size_t gpu_index, size_t screen_index) {
    return &this->screen_groups[gpu_index].screens[screen_index];
}

void DirectDisplay::poll_events() {
    this->input.poll_events();
}

void DirectDisplay::swap_buffers() {
    for (auto& screen_group : this->screen_groups) {
        screen_group.swap_buffers();
    }
}