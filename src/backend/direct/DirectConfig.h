#ifndef _XENODON_BACKEND_DIRECT_DIRECTCONFIG_H
#define _XENODON_BACKEND_DIRECT_DIRECTCONFIG_H

#include <string>
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include "core/Config.h"

struct DirectConfig {
    struct Output {
        uint32_t vulkan_index;
        vk::Offset2D offset;
    };

    struct Device {
        uint32_t vulkan_index;
        std::vector<Output> outputs;
    };

    struct Input {
        std::string kbd_dev;
    };

    std::vector<Device> gpus;
    Input input;
};

template<>
struct cfg::FromConfig<DirectConfig::Output> {
    DirectConfig::Output operator()(cfg::Config& cfg) const;
};

template<>
struct cfg::FromConfig<DirectConfig::Device> {
    DirectConfig::Device operator()(cfg::Config& cfg) const;
};

template<>
struct cfg::FromConfig<DirectConfig::Input> {
    DirectConfig::Input operator()(cfg::Config& cfg) const;
};

template<>
struct cfg::FromConfig<DirectConfig> {
    DirectConfig operator()(cfg::Config& cfg) const;
};

#endif
