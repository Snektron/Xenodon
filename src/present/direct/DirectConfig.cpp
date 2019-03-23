#include "present/direct/DirectConfig.h"
#include <algorithm>

using Screen = DirectConfig::Screen;
using Device = DirectConfig::Device;
using Input = DirectConfig::Input;

vk::Offset2D cfg::Parse<vk::Offset2D>::operator()(cfg::Parser& p) {
    p.expect('(');
    p.optws();
    size_t x = cfg::Parse<size_t>{}(p);
    p.optws();
    p.expect(',');
    p.optws();
    size_t y = cfg::Parse<size_t>{}(p);
    p.optws();
    p.expect(')');

    return {
        static_cast<int32_t>(x),
        static_cast<int32_t>(y)
    };
}

Screen cfg::FromConfig<Screen>::operator()(cfg::Config& cfg) {
    auto [index, offset] = cfg.get(
        Value<size_t>("vkindex"),
        Value<vk::Offset2D>("offset")
    );

    return {
        static_cast<uint32_t>(index),
        offset
    };
}

Device cfg::FromConfig<Device>::operator()(cfg::Config& cfg) {
    auto [index, screens] = cfg.get(
        Value<size_t>("vkindex"),
        Vector<Screen>("screen")
    );

    if (screens.empty()) {
        throw cfg::ConfigError("Config error: A device must have at least one screen entry");
    }

    std::sort(screens.begin(), screens.end(), [](auto& a, auto& b) {
        return a.vulkan_index < b.vulkan_index;
    });

    auto it = adjacent_find(screens.begin(), screens.end(), [](auto& a, auto& b) {
        return a.vulkan_index == b.vulkan_index;
    });

    if (it != screens.end()) {
        throw cfg::ConfigError("Config error: Screen vulkan indices within a device must be unique");
    }

    return {
        static_cast<uint32_t>(index),
        screens
    };
}

Input cfg::FromConfig<Input>::operator()(cfg::Config& cfg) {
    auto [kbd_dev] = cfg.get(
        Value<std::string>("keyboard")
    );

    return {kbd_dev};
}

DirectConfig cfg::FromConfig<DirectConfig>::operator()(cfg::Config& cfg) {
    auto [devices, input] = cfg.get(
        Vector<Device>("device"),
        Struct<Input>("input")
    );

    if (devices.empty()) {
        throw cfg::ConfigError("Config error: The config must have at least one device entry");
    }

    return {devices, input};
}
