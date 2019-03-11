#include "graphics/DisplayConfig.h"
#include <optional>

namespace {
    using PlanePropertiesVector = std::vector<vk::DisplayPlanePropertiesKHR>;
    using DisplayInfo = DisplayConfig::DisplayInfo;

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

    std::vector<DisplayInfo> detect_displays(vk::PhysicalDevice gpu) {
        auto displays = std::vector<DisplayInfo>();

        auto all_display_props = gpu.getDisplayPropertiesKHR();

        for (size_t display_index = 0; display_index < all_display_props.size(); ++display_index) {
            auto& display_props = all_display_props[display_index];
            auto mode_props = gpu.getDisplayModePropertiesKHR(display_props.display).front();
            auto plane_props = gpu.getDisplayPlanePropertiesKHR();
            auto plane_index = find_plane_index(gpu, plane_props, display_props.display);
            if (!plane_index)
                continue;

            displays.emplace_back(DisplayInfo{
                .index = static_cast<uint32_t>(display_index),
                .name = display_props.displayName,
                .region = vk::Rect2D{{0, 0}, mode_props.parameters.visibleRegion},
                .display = display_props.display,
                .display_mode = mode_props.displayMode,
                .plane_index = plane_index.value(),
                .plane_stack_index = plane_props[plane_index.value()].currentStackIndex
            });
        }

        return displays;
    }
}

DisplayConfig DisplayConfig::auto_detect(vk::Instance instance) {
    auto dc = DisplayConfig();
    dc.extent = vk::Extent2D{0, 0};

    auto gpus = instance.enumeratePhysicalDevices();
    for (size_t gpu_index = 0; gpu_index < gpus.size(); ++gpu_index) {
        auto gpu = gpus[gpu_index];
        auto props = gpu.getProperties();

        auto displays = detect_displays(gpu);

        if (displays.empty()) {
            continue;
        }

        dc.gpus.emplace_back(GpuInfo{
            .index = static_cast<uint32_t>(gpu_index),
            .name = props.deviceName,
            .region = vk::Rect2D{{0, 0}, {0, 0}},
            .gpu = gpu,
            .displays = std::move(displays)
        });
    }

    return dc;
}

DisplayConfig DisplayConfig::parse_from_config(vk::Instance instance, std::istream& config) {
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
            << r.offset.x << ", " << r.offset.y << " to "
            << (r.offset.x + static_cast<int32_t>(r.extent.width)) << ", " 
            << (r.offset.y + static_cast<int32_t>(r.extent.height)) << " ("
            << r.extent.width << 'x' << r.extent.height << ')';
    };

    auto str = [&](const char* s) -> std::ostream& {
        return os << (s ? s : "(null)");
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

        for (auto& display_info : gpu_info.displays) {
            indent(2) << "display:\n";
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