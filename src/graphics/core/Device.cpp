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