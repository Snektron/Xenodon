#include "graphics/core/Instance.h"
#include <algorithm>
#include "version.h"

Instance::Instance(Span<const char* const> extensions):
    instance(vk::createInstanceUnique({
        {},
        &version::APP_INFO,
        0,
        nullptr,
        static_cast<uint32_t>(extensions.size()),
        extensions.data()
    })) {

    auto vkdevs = this->instance->enumeratePhysicalDevices();
    this->physdevs.reserve(vkdevs.size());

    std::transform(
        vkdevs.begin(),
        vkdevs.end(),
        std::back_inserter(this->physdevs),
        [](vk::PhysicalDevice device) {
            return PhysicalDevice(device);
        }
    );
}
