#include "graphics/Device.h"

namespace {
    vk::UniqueDevice create_logical_device(vk::PhysicalDevice gpu, vk::ArrayProxy<const char* const> extensions, uint32_t gqi, uint32_t pqi) {
        auto logical_info = vk::DeviceCreateInfo();
        logical_info.queueCreateInfoCount = 1;

        // Create the logical device
        float priority = 1.0f;

        auto queue_infos = std::array<vk::DeviceQueueCreateInfo, 2>();

        queue_infos[0].flags = {};
        queue_infos[0].queueFamilyIndex = gqi;
        queue_infos[0].queueCount = 1;
        queue_infos[0].pQueuePriorities = &priority;

        if (gqi != pqi) {
            logical_info.queueCreateInfoCount = 2;

            queue_infos[1].flags = {};
            queue_infos[1].queueFamilyIndex = pqi;
            queue_infos[1].queueCount = 1;
            queue_infos[1].pQueuePriorities = &priority;
        }

        logical_info.pQueueCreateInfos = queue_infos.data();
        logical_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        logical_info.ppEnabledExtensionNames = extensions.data();

        return gpu.createDeviceUnique(logical_info);
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

Device::Device(vk::PhysicalDevice physical, vk::ArrayProxy<const char* const> extensions, uint32_t graphics_queue, uint32_t present_queue):
    physical(physical),
    logical(create_logical_device(physical, extensions, graphics_queue, present_queue)),
    graphics(logical.get(), graphics_queue),
    present(logical.get(), present_queue),
    graphics_command_pool(create_command_pool(this->logical.get(), graphics_queue)) {
}
