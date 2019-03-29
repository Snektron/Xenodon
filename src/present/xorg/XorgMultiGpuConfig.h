#ifndef _XENODON_PRESENT_XORG_XORGMULTIGPUCONFIG_H
#define _XENODON_PRESENT_XORG_XORGMULTIGPUCONFIG_H

#include <vector>
#include <string>
#include <vulkan/vulkan.hpp>
#include "core/Config.h"

struct XorgMultiGpuConfig {
    struct Screen {
        std::string displayname;
        vk::Offset2D offset;
    };

    std::vector<Screen> screens;
};

template<>
struct cfg::FromConfig<XorgMultiGpuConfig::Screen> {
    XorgMultiGpuConfig::Screen operator()(cfg::Config& cfg) const;
};

template<>
struct cfg::FromConfig<XorgMultiGpuConfig> {
    XorgMultiGpuConfig operator()(cfg::Config& cfg) const;
};

#endif
