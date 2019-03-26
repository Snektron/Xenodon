#include "present/headless/HeadlessConfig.h"

using Device = HeadlessConfig::Device;

Device cfg::FromConfig<Device>::operator()(cfg::Config& cfg) const {
    auto [index, offset, extent] = cfg.get(
        Value<size_t>("vkindex"),
        Value<vk::Offset2D>("offset"),
        Value<vk::Extent2D>("extent")
    );

    return {
        static_cast<uint32_t>(index),
        {offset, extent}
    };
}

HeadlessConfig cfg::FromConfig<HeadlessConfig>::operator()(cfg::Config& cfg) const {
    auto [devices] = cfg.get(
        Vector<Device>("device")
    );

    if (devices.empty()) {
        throw cfg::ConfigError("At least one device entry is required");
    }

    return {devices};
}