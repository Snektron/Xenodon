#ifndef _XENODON_PRESENT_DIRECT_DIRECTCONFIG_H
#define _XENODON_PRESENT_DIRECT_DIRECTCONFIG_H

#include <string>
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include "core/Config.h"
#include "core/Parser.h"

struct DirectConfig {
    struct Screen {
        uint32_t vulkan_index;
        vk::Offset2D offset;
    };

    struct Device {
        uint32_t vulkan_index;
        std::vector<Screen> screens;
    };

    struct Input {
        std::string kbd_dev;
    };

    std::vector<Device> gpus;
    Input input;
};

template<>
struct parser::Parse<vk::Offset2D> {
    vk::Offset2D operator()(parser::Parser& p);
};

template<>
struct cfg::FromConfig<DirectConfig::Screen> {
    DirectConfig::Screen operator()(cfg::Config& cfg);
};

template<>
struct cfg::FromConfig<DirectConfig::Device> {
    DirectConfig::Device operator()(cfg::Config& cfg);
};

template<>
struct cfg::FromConfig<DirectConfig::Input> {
    DirectConfig::Input operator()(cfg::Config& cfg);
};

template<>
struct cfg::FromConfig<DirectConfig> {
    DirectConfig operator()(cfg::Config& cfg);
};

#endif
