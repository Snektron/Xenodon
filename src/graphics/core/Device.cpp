#include "graphics/core/Device.h"
#include <vector>
#include <algorithm>

Device::Device(const PhysicalDevice& physdev, Span<vk::DeviceQueueCreateInfo> queue_families, Span<const char*> extensions):
    physdev(physdev.get()) {

    this->dev = this->physdev.createDeviceUnique({
        {},
        static_cast<uint32_t>(queue_families.size()),
        queue_families.data(),
        0,
        nullptr,
        static_cast<uint32_t>(extensions.size()),
        extensions.data()
    });
}

Device::Device(const PhysicalDevice& physdev, Span<uint32_t> queue_families, Span<const char*> extensions):
    physdev(physdev.get()) {
    float priority = 1.0f;
    auto queue_create_infos = std::vector<vk::DeviceQueueCreateInfo>(queue_families.size());

    std::transform(
        queue_families.begin(),
        queue_families.end(),
        queue_create_infos.begin(),
        [&priority](uint32_t family) {
            return vk::DeviceQueueCreateInfo {
                {},
                family,
                1,
                &priority
            };
        }
    );

    std::sort(
        queue_create_infos.begin(),
        queue_create_infos.end(),
        [](const auto& a, const auto& b){
            return a.queueFamilyIndex < b.queueFamilyIndex;
        }
    );

    const auto last = std::unique(queue_create_infos.begin(), queue_create_infos.end());

    this->dev = this->physdev.createDeviceUnique({
        {},
        static_cast<uint32_t>(std::distance(queue_create_infos.begin(), last)),
        queue_create_infos.data(),
        0,
        nullptr,
        static_cast<uint32_t>(extensions.size()),
        extensions.data(),
        nullptr
    });
}

std::optional<uint32_t> Device::find_memory_type(uint32_t filter, vk::MemoryPropertyFlags flags) const {
    auto mem_props = this->physdev.getMemoryProperties();
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
        if (filter & (1 << i) && (mem_props.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }

    return std::nullopt;
}

vk::DeviceMemory Device::allocate(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags flags) const {
    auto alloc_info = vk::MemoryAllocateInfo(
        requirements.size,
        this->find_memory_type(requirements.memoryTypeBits, flags).value()
    );

    return this->dev->allocateMemory(alloc_info);
}

vk::UniqueDeviceMemory Device::allocate_unique(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags flags) const {
    auto alloc_info = vk::MemoryAllocateInfo(
        requirements.size,
        this->find_memory_type(requirements.memoryTypeBits, flags).value()
    );

    return this->dev->allocateMemoryUnique(alloc_info);
}
