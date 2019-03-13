#ifndef _XENODON_GRAPHICS_DISPLAYCONFIG_H
#define _XENODON_GRAPHICS_DISPLAYCONFIG_H

#include <vector>
#include <istream>
#include <ostream>
#include <cstdint>
#include <vulkan/vulkan.hpp>

struct DisplayConfig {
    struct ScreenInfo {
        uint32_t index;
        const char* name;
        vk::Rect2D region;
        vk::DisplayKHR display;
        vk::DisplayModeKHR display_mode;
        uint32_t plane_index;
        uint32_t plane_stack_index;
    };

    struct GpuInfo {
        uint32_t index;
        const char* name;
        vk::Rect2D region;
        vk::PhysicalDevice gpu;
        std::vector<ScreenInfo> screens;
    };

    enum class Direction {
        Horizontal,
        Vertical
    };

    vk::Extent2D extent;
    std::vector<GpuInfo> gpus;

private:
    DisplayConfig() = default;

public:
    static DisplayConfig auto_detect(vk::Instance instance, Direction dir = Direction::Horizontal);
    static DisplayConfig parse_config(vk::Instance instance, std::istream& input);
};

std::ostream& operator<<(std::ostream& os, const DisplayConfig& dc);

#endif
