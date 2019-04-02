#include "present/xorg/XorgMultiGpuConfig.h"

using Output = XorgMultiGpuConfig::Output;

Output cfg::FromConfig<Output>::operator()(cfg::Config& cfg) const {
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
    auto [outputs] = cfg.get(
        Vector<Output>("output")
    );

    if (outputs.empty()) {
        throw cfg::ConfigError("At least one output entry is required");
    }

    return {outputs};
}
