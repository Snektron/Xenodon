#include "graphics/Device.h"

namespace {
    vk::UniqueDevice create_logical_device(vk::PhysicalDevice gpu, vk::ArrayProxy<const char* const> extensions, uint32_t gqi) {
        float priority = 1.0f;

        auto queue_create_info = vk::DeviceQueueCreateInfo(
            {},
            gqi,
            1,
            &priority
        );

        auto logical_create_info = vk::DeviceCreateInfo(
            {},
            1,
            &queue_create_info,
            0,
            nullptr,
            static_cast<uint32_t>(extensions.size()),
            extensions.data(),
            nullptr
        );

        return gpu.createDeviceUnique(logical_create_info);
    }

    vk::UniqueCommandPool create_command_pool(vk::Device logical, uint32_t gqi) {
        auto pool_info = vk::CommandPoolCreateInfo();
        pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        pool_info.queueFamilyIndex = gqi;

        return logical.createCommandPoolUnique(pool_info);
    }
}

Queue::Queue(vk::Device device, uint32_t family_index):
    queue(device.getQueue(family_index, 0)),
    family_index(family_index) {
}

Queue::Queue(std::nullptr_t):
    queue(vk::Queue(nullptr)),
    family_index(std::numeric_limits<uint32_t>::max()) {
}

bool Queue::is_valid() const {
    return this->queue != vk::Queue(nullptr);
}

Device::Device(vk::PhysicalDevice physical, vk::ArrayProxy<const char* const> extensions, uint32_t graphics_queue):
    physical(physical),
    logical(create_logical_device(physical, extensions, graphics_queue)),
    graphics(logical.get(), graphics_queue),
    graphics_command_pool(create_command_pool(this->logical.get(), graphics_queue)) {
}
