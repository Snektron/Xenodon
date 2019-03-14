#ifndef _XENODON_PRESENT_DIRECT_DISPLAYCONFIG_H
#define _XENODON_PRESENT_DIRECT_DISPLAYCONFIG_H

#include <cstdint>
#include <vector>
#include <istream>
#include <stdexcept>
#include <string_view>
#include <vulkan/vulkan.hpp>

struct DisplayConfig {
    struct ParseError: public std::runtime_error {
        ParseError(std::string_view msg):
            runtime_error(std::string(msg)) {
        }
    };

    struct Screen {
        uint32_t vulkan_index;
        vk::Offset2D offset;
    };

    struct Device {
        uint32_t vulkan_index;
        std::vector<Screen> screens;
    };

    std::vector<Device> gpus;

    DisplayConfig(std::istream& input);
};

#endif