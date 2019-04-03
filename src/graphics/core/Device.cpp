#include "graphics/core/Device.h"
#include <vector>
#include <algorithm>

Device2::Device2(const PhysicalDevice& physdev, Span<vk::DeviceQueueCreateInfo> queue_families, Span<const char* const> extensions):
    physdev(physdev.get()) {
    this->dev = this->physdev.createDeviceUnique({
        {},
        static_cast<uint32_t>(queue_families.size()),
        queue_families.data(),
        0,
        nullptr,
        static_cast<uint32_t>(extensions.size()),
        extensions.data(),
        nullptr
    });
}

Device2::Device2(const PhysicalDevice& physdev, Span<uint32_t> queue_families, Span<const char* const> extensions):
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

    this->dev = this->physdev.createDeviceUnique({
        {},
        static_cast<uint32_t>(queue_create_infos.size()),
        queue_create_infos.data(),
        0,
        nullptr,
        static_cast<uint32_t>(extensions.size()),
        extensions.data(),
        nullptr
    });
}

std::optional<uint32_t> Device2::find_memory_type(uint32_t filter, vk::MemoryPropertyFlags flags) const {
    auto mem_props = this->physdev.getMemoryProperties();
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
        if (filter & (1 << i) && (mem_props.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }

    return std::nullopt;
}

vk::UniqueDeviceMemory Device2::allocate(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags flags) const {
    auto alloc_info = vk::MemoryAllocateInfo(
        requirements.size,
        this->find_memory_type(requirements.memoryTypeBits, flags).value()
    );

    return this->dev->allocateMemoryUnique(alloc_info);
}
