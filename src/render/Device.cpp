#include "render/Device.h"

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
    graphics(nullptr),
    present(nullptr) {

    auto logical_info = vk::DeviceCreateInfo();
    logical_info.queueCreateInfoCount = 1;

    // Create the logical device
    float priority = 1.0f;

    auto queue_infos = std::array<vk::DeviceQueueCreateInfo, 2>();

    queue_infos[0].flags = {};
    queue_infos[0].queueFamilyIndex = graphics_queue;
    queue_infos[0].queueCount = 1;
    queue_infos[0].pQueuePriorities = &priority;

    if (graphics_queue != present_queue) {
        logical_info.queueCreateInfoCount = 2;

        queue_infos[1].flags = {};
        queue_infos[1].queueFamilyIndex = present_queue;
        queue_infos[1].queueCount = 1;
        queue_infos[1].pQueuePriorities = &priority;
    }

    logical_info.pQueueCreateInfos = queue_infos.data();
    logical_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    logical_info.ppEnabledExtensionNames = extensions.data();

    auto logical = this->physical.createDeviceUnique(logical_info);

    // Create the device queues
    this->graphics = Queue(logical.get(), graphics_queue);
    this->present = Queue(logical.get(), present_queue);

    // Create the device's command pool
    {
        auto pool_info = vk::CommandPoolCreateInfo();
        pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        pool_info.queueFamilyIndex = graphics_queue;

        this->graphics_command_pool = logical->createCommandPoolUnique(pool_info);
    };

    this->logical = std::move(logical); // Set the logical device here so it gets destroyed if createCommandPoolUnique throws
}
