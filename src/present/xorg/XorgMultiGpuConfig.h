#ifndef _XENODON_PRESENT_XORG_XORGMULTIGPUCONFIG_H
#define _XENODON_PRESENT_XORG_XORGMULTIGPUCONFIG_H

#include <vector>
#include <string>
#include <vulkan/vulkan.hpp>
#include "core/Config.h"

struct XorgMultiGpuConfig {
    struct Output {
        std::string displayname;
        vk::Offset2D offset;
    };

    std::vector<Output> outputs;
};

template<>
struct cfg::FromConfig<XorgMultiGpuConfig::Output> {
    XorgMultiGpuConfig::Output operator()(cfg::Config& cfg) const;
};

template<>
struct cfg::FromConfig<XorgMultiGpuConfig> {
    XorgMultiGpuConfig operator()(cfg::Config& cfg) const;
};

#endif
