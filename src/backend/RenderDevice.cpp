#include "backend/RenderDevice.h"

namespace {
    vk::UniqueCommandPool create_command_pool(Device2& device, uint32_t family) {
        auto pool_info = vk::CommandPoolCreateInfo();
        pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        pool_info.queueFamilyIndex = family;

        return device->createCommandPoolUnique(pool_info);
    }
}

RenderDevice::RenderDevice(Device2&& device, uint32_t graphics_queue_family, size_t outputs):
    device(std::move(device)),
    graphics_queue(this->device, graphics_queue_family),
    outputs(outputs),
    graphics_command_pool(create_command_pool(this->device, graphics_queue_family)) {
}
