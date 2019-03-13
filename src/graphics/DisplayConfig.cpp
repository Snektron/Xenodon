#include "graphics/DisplayConfig.h"
#include <optional>
#include <algorithm>
#include <stdexcept>

using PlanePropertiesVector = std::vector<vk::DisplayPlanePropertiesKHR>;
using ScreenInfo = DisplayConfig::ScreenInfo;
using GpuInfo = DisplayConfig::GpuInfo;
using Direction = DisplayConfig::Direction;

namespace {
    std::optional<uint32_t> find_plane_index(vk::PhysicalDevice gpu, const PlanePropertiesVector& props, vk::DisplayKHR display) {
        for (size_t plane = 0; plane < props.size(); ++plane) {
            if (props[plane].currentDisplay != vk::DisplayKHR(nullptr) && props[plane].currentDisplay != display)
                continue;

            auto supported_displays = gpu.getDisplayPlaneSupportedDisplaysKHR(static_cast<uint32_t>(plane));
            if (supported_displays.size() == 0)
                continue;

            for (uint32_t i = 0; i < supported_displays.size(); ++i) {
                if (supported_displays[i] == display) {
                    return static_cast<uint32_t>(plane);
                }
            }
        }

        return std::nullopt;
    }

    std::vector<ScreenInfo> detect_screens(vk::PhysicalDevice gpu, Direction dir, vk::Offset2D offset) {
        auto screens = std::vector<ScreenInfo>();

        auto all_display_props = gpu.getDisplayPropertiesKHR();

        for (size_t display_index = 0; display_index < all_display_props.size(); ++display_index) {
            auto& display_props = all_display_props[display_index];
            auto mode_props = gpu.getDisplayModePropertiesKHR(display_props.display).front();
            auto plane_props = gpu.getDisplayPlanePropertiesKHR();
            auto plane_index = find_plane_index(gpu, plane_props, display_props.display);
            if (!plane_index)
                continue;

            screens.emplace_back(ScreenInfo{
                .index = static_cast<uint32_t>(display_index),
                .name = display_props.displayName,
                .region = vk::Rect2D{offset, mode_props.parameters.visibleRegion},
                .display = display_props.display,
                .display_mode = mode_props.displayMode,
                .plane_index = plane_index.value(),
                .plane_stack_index = plane_props[plane_index.value()].currentStackIndex
            });

            if (dir == Direction::Horizontal) {
                offset.x += static_cast<int32_t>(mode_props.parameters.visibleRegion.width);
            } else {
                offset.y += static_cast<int32_t>(mode_props.parameters.visibleRegion.height);
            }
        }

        return screens;
    }

    template <typename T, typename F>
    vk::Rect2D surrounding_region(const std::vector<T>& container, F get_region) {
        if (container.empty())
            return vk::Rect2D{{0, 0}, {0, 0}};

        auto first_region = get_region(container[0]);
        uint32_t startx = static_cast<uint32_t>(first_region.offset.x);
        uint32_t starty = static_cast<uint32_t>(first_region.offset.y);
        uint32_t endx = startx + first_region.extent.width;
        uint32_t endy = starty + first_region.extent.height;

        for (size_t i = 1; i < container.size(); ++i) {
            auto [offset, extent] = get_region(container[i]);

            startx = std::min(startx, static_cast<uint32_t>(offset.x));
            starty = std::min(starty, static_cast<uint32_t>(offset.y));

            endx = std::max(endx, static_cast<uint32_t>(offset.x) + extent.width);
            endy = std::max(endy, static_cast<uint32_t>(offset.y) + extent.height);
        }

        return vk::Rect2D{
            {
                static_cast<int32_t>(startx),
                static_cast<int32_t>(starty)
            },
            {
                endx - startx,
                endy - starty
            }
        };
    }

    size_t parse_uint(std::istream& in) {
        int c = in.peek();
        if (c < '0' || c > '9') {
            throw std::runtime_error("Expected uint");
        }

        size_t value = static_cast<size_t>(c);

        c = in.peek();
        while (c >= '0' && c <= '9') {
            value *= 10;
            value += static_cast<size_t>(c);
        }

        return value;
    }
}

DisplayConfig DisplayConfig::auto_detect(vk::Instance instance, Direction dir) {
    auto dc = DisplayConfig();

    auto gpus = instance.enumeratePhysicalDevices();
    auto offset = vk::Offset2D{0, 0};

    for (size_t gpu_index = 0; gpu_index < gpus.size(); ++gpu_index) {
        auto gpu = gpus[gpu_index];
        auto props = gpu.getProperties();

        auto screens = detect_screens(gpu, dir, offset);

        if (screens.empty()) {
            continue;
        }

        auto region = surrounding_region(screens, [](auto& screens) {
            return screens.region;
        });

        dc.gpus.emplace_back(GpuInfo{
            .index = static_cast<uint32_t>(gpu_index),
            .name = props.deviceName,
            .region = region,
            .gpu = gpu,
            .screens = std::move(screens)
        });

        if (dir == Direction::Vertical) {
            offset.x += static_cast<int32_t>(region.extent.width);
        } else {
            offset.y += static_cast<int32_t>(region.extent.height);
        }
    }

    // the offset is {0, 0}
    dc.extent = surrounding_region(dc.gpus, [](auto& gpu) {
        return gpu.region;
    }).extent;

    return dc;
}

DisplayConfig DisplayConfig::parse_config(vk::Instance instance, std::istream& input) {
    return DisplayConfig();
}

std::ostream& operator<<(std::ostream& os, const DisplayConfig& dc) {
    auto indent = [&](size_t n) -> std::ostream& {
        for (size_t i = 0; i < n; ++i) {
            os << "    ";
        }

        return os;
    };

    auto rect = [&](vk::Rect2D r) -> std::ostream& {
        return os
            << '(' << r.offset.x << ", " << r.offset.y << ") to ("
            << (r.offset.x + static_cast<int32_t>(r.extent.width)) << ", " 
            << (r.offset.y + static_cast<int32_t>(r.extent.height)) << ") ("
            << r.extent.width << " by " << r.extent.height << " pixels)";
    };

    auto str = [&](const char* s) -> std::ostream& {
        if (s) {
            return os << '\'' << s << '\'';
        } else {
            return os << "(null)";
        }
    };

    os << "Display configuration:\n";
    indent(1) << "extent: " << dc.extent.width << 'x' << dc.extent.height << '\n';
    for (auto& gpu_info : dc.gpus) {
        indent(1) << "gpu:\n";
        indent(2) << "vulkan index: " << gpu_info.index << "\n";
        indent(2) << "name: ";
        str(gpu_info.name) << '\n';
        indent(2) << "region: ";
        rect(gpu_info.region) << '\n';

        for (auto& display_info : gpu_info.screens) {
            indent(2) << "screen:\n";
            indent(3) << "vulkan index: " << display_info.index << "\n";
            indent(3) << "name: ";
            str(display_info.name) << '\n';
            indent(3) << "region: ";
            rect(display_info.region) << '\n';
            indent(3) << "plane index: " << display_info.plane_index << '\n';
            indent(3) << "plane stack index: " << display_info.plane_stack_index << '\n';
        }
    } 

    return os;
}