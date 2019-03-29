#include "present/xorg/XorgMultiGpuConfig.h"

using Screen = XorgMultiGpuConfig::Screen;

Screen cfg::FromConfig<Screen>::operator()(cfg::Config& cfg) const {
    auto [displayname, offset] = cfg.get(
        Value<std::string>("displayname"),
        Value<vk::Offset2D>("offset")
    );

    return {
        displayname,
        offset,
    };
}

XorgMultiGpuConfig cfg::FromConfig<XorgMultiGpuConfig>::operator()(cfg::Config& cfg) const {
    auto [screens] = cfg.get(
        Vector<Screen>("screen")
    );

    if (screens.empty()) {
        throw cfg::ConfigError("At least one screen entry is required");
    }

    return {screens};
}
