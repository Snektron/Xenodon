#include "present/direct/DirectDisplay.h"
#include <array>
#include "core/Logger.h"
#include "version.h"

namespace {
    constexpr const std::array REQUIRED_INSTANCE_EXTENSIONS = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_DISPLAY_EXTENSION_NAME,
        VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME
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

DirectDisplay::DirectDisplay(EventDispatcher& dispatcher, const DirectConfig& display_config):
    instance(vk::createInstanceUnique(INSTANCE_CREATE_INFO)),
    input(dispatcher, display_config.input) {
    this->screen_groups.reserve(display_config.gpus.size());

    auto gpus = this->instance->enumeratePhysicalDevices();
    for (const auto& device : display_config.gpus) {
        if (device.vulkan_index >= gpus.size()) {
            throw std::runtime_error("Vulkan screen index out of range");
        }

        this->screen_groups.emplace_back(this->instance.get(), gpus[device.vulkan_index], device.screens);
    }

    for (size_t i = 0; i < this->screen_groups.size(); ++i) {
        for (size_t j = 0; j < this->screen_groups[i].screens.size(); ++j) {
            LOGGER.log("Screen {} {} info:", i, j);
            const auto& swapchain = this->screen_groups[i].screens[j].swapchain;

            LOGGER.log("\tPresent mode: {}", vk::to_string(swapchain.surface_present_mode()));
            auto extent = swapchain.surface_extent();
            LOGGER.log("\tExtent: {}x{}", extent.width, extent.height);
            auto surface_format = swapchain.surface_format();
            LOGGER.log("\tFormat: {}", vk::to_string(surface_format.format));
            LOGGER.log("\tColorspace: {}", vk::to_string(surface_format.colorSpace));
            LOGGER.log("\tSwapchain images: {}", swapchain.num_images());
        }
    }
}

Setup DirectDisplay::setup() const {
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
