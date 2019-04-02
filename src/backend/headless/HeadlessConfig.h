#ifndef _XENODON_PRESENT_HEADLESS_HEADLESSCONFIG_H
#define _XENODON_PRESENT_HEADLESS_HEADLESSCONFIG_H

#include <vector>
#include <vulkan/vulkan.hpp>
#include "core/Config.h"

struct HeadlessConfig {
    struct Device {
        uint32_t vulkan_index;
        vk::Rect2D region;
    };

    std::vector<Device> gpus;
};

template<>
struct cfg::FromConfig<HeadlessConfig::Device> {
    HeadlessConfig::Device operator()(cfg::Config& cfg) const;
};

template<>
struct cfg::FromConfig<HeadlessConfig> {
    HeadlessConfig operator()(cfg::Config& cfg) const;
};

#endif
