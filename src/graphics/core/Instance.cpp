#include "graphics/core/Instance.h"
#include <algorithm>
#include <cstring>
#include "core/Error.h"
#include "version.h"

Instance::Instance(Span<const char* const> extensions) {
    auto supported_extensions = vk::enumerateInstanceExtensionProperties();

    for (const char* ext_name : extensions) {
        auto cmp_ext = [&ext_name](auto& properties) {
            return std::strcmp(ext_name, properties.extensionName) == 0;
        };

        if (std::none_of(supported_extensions.begin(), supported_extensions.end(), cmp_ext)) {
            throw Error("Unsupported extension: {}", ext_name);
        }
    }

    this->instance = vk::createInstanceUnique({
        {},
        &version::APP_INFO,
        0,
        nullptr,
        static_cast<uint32_t>(extensions.size()),
        extensions.data()
    });

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
