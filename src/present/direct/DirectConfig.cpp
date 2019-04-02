#include "present/direct/DirectConfig.h"
#include <algorithm>

using Output = DirectConfig::Output;
using Device = DirectConfig::Device;
using Input = DirectConfig::Input;

Output cfg::FromConfig<Output>::operator()(cfg::Config& cfg) const {
    auto [index, offset] = cfg.get(
        Value<size_t>("vkindex"),
        Value<vk::Offset2D>("offset")
    );

    return {
        static_cast<uint32_t>(index),
        offset
    };
}

Device cfg::FromConfig<Device>::operator()(cfg::Config& cfg) const {
    auto [index, outputs] = cfg.get(
        Value<size_t>("vkindex"),
        Vector<Output>("output")
    );

    if (outputs.empty()) {
        throw cfg::ConfigError("A device must have at least one output entry");
    }

    std::sort(outputs.begin(), outputs.end(), [](auto& a, auto& b) {
        return a.vulkan_index < b.vulkan_index;
    });

    auto it = adjacent_find(outputs.begin(), outputs.end(), [](auto& a, auto& b) {
        return a.vulkan_index == b.vulkan_index;
    });

    if (it != outputs.end()) {
        throw cfg::ConfigError("Output vulkan indices within a device must be unique");
    }

    return {
        static_cast<uint32_t>(index),
        outputs
    };
}

Input cfg::FromConfig<Input>::operator()(cfg::Config& cfg) const {
    auto [kbd_dev] = cfg.get(
        Value<std::string>("keyboard")
    );

    return {kbd_dev};
}

DirectConfig cfg::FromConfig<DirectConfig>::operator()(cfg::Config& cfg) const {
    auto [devices, input] = cfg.get(
        Vector<Device>("device"),
        Struct<Input>("input")
    );

    if (devices.empty()) {
        throw cfg::ConfigError("At least one device entry is required");
    }

    return {devices, input};
}
