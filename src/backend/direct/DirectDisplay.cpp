#include "backend/direct/DirectDisplay.h"
#include <array>
#include "core/Logger.h"
#include "core/Error.h"

namespace {
    constexpr const std::array REQUIRED_INSTANCE_EXTENSIONS = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_DISPLAY_EXTENSION_NAME,
        VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME
    };
}

DirectDisplay::DirectDisplay(EventDispatcher& dispatcher, const DirectConfig& display_config):
    instance(REQUIRED_INSTANCE_EXTENSIONS),
    input(dispatcher, display_config.input) {

    const auto& gpus = this->instance.physical_devices();
    this->screen_groups.reserve(display_config.gpus.size());

    for (const auto& device : display_config.gpus) {
        if (device.vulkan_index >= gpus.size()) {
            throw Error("Vulkan device index {} out of range", device.vulkan_index);
        }

        this->screen_groups.push_back(std::make_unique<ScreenGroup>(this->instance, gpus[device.vulkan_index], device.outputs));
    }

    for (size_t i = 0; i < this->screen_groups.size(); ++i) {
        LOGGER.log("Render device {}:", i);
        this->screen_groups[i]->log();

        // for (size_t j = 0; j < this->screen_groups[i].outputs.size(); ++j) {
        //     LOGGER.log("Screen {} {} info:", i, j);
        //     const auto& swapchain = this->screen_groups[i].outputs[j].swapchain;

        //     LOGGER.log("\tPresent mode: {}", vk::to_string(swapchain.surface_present_mode()));
        //     auto extent = swapchain.surface_extent();
        //     LOGGER.log("\tExtent: {}x{}", extent.width, extent.height);
        //     auto surface_format = swapchain.surface_format();
        //     LOGGER.log("\tFormat: {}", vk::to_string(surface_format.format));
        //     LOGGER.log("\tColorspace: {}", vk::to_string(surface_format.colorSpace));
        //     LOGGER.log("\tSwapchain images: {}", swapchain.num_images());
        // }
    }
}

size_t DirectDisplay::num_render_devices() const {
    return this->screen_groups.size();
}

const RenderDevice& DirectDisplay::render_device(size_t device_index) {
    return this->screen_groups[device_index]->render_device();
}

Output* DirectDisplay::output(size_t device_index, size_t output_index) {
    return &this->screen_groups[device_index]->output(output_index);
}

void DirectDisplay::swap_buffers()  {
    for (auto& screen_group : this->screen_groups) {
        screen_group->swap_buffers();
    }
}

void DirectDisplay::poll_events() {
    this->input.poll_events();
}
