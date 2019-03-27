#include "present/direct/ScreenGroup.h"
#include <array>
#include "graphics/utility.h"

namespace {
    constexpr const std::array DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_DISPLAY_CONTROL_EXTENSION_NAME
    };

    vk::UniqueSurfaceKHR create_surface(vk::Instance instance, vk::PhysicalDevice gpu, vk::DisplayKHR display) {
        vk::DisplayModePropertiesKHR mode_props;
        uint32_t property_count = 1;

        vk::createResultValue(
            gpu.getDisplayModePropertiesKHR(
                display,
                &property_count,
                &mode_props
            ),
            __PRETTY_FUNCTION__,
            {vk::Result::eSuccess, vk::Result::eIncomplete}
        );

        auto plane_props = gpu.getDisplayPlanePropertiesKHR();

        uint32_t plane;
        bool found = false;
        for (plane = 0; plane < plane_props.size(); ++plane) {
            if (plane_props[plane].currentDisplay != vk::DisplayKHR(nullptr) && plane_props[plane].currentDisplay != display)
                continue;

            auto supported_displays = gpu.getDisplayPlaneSupportedDisplaysKHR(plane);
            if (supported_displays.size() == 0)
                continue;

            for (uint32_t i = 0; i < supported_displays.size(); ++i) {
                if (supported_displays[i] == display) {
                    found = true;
                }
            }

            if (found)
                break;
        }

        if (!found)
            throw std::runtime_error("Failed to find compatible plane");

        auto create_info = vk::DisplaySurfaceCreateInfoKHR();
        create_info.flags = {};
        create_info.displayMode = mode_props.displayMode;
        create_info.planeIndex = plane;
        create_info.planeStackIndex = plane_props[plane].currentStackIndex;
        create_info.transform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
        create_info.alphaMode = vk::DisplayPlaneAlphaFlagBitsKHR::eOpaque;
        create_info.globalAlpha = 1.0f;
        create_info.imageExtent = mode_props.parameters.visibleRegion;

        return instance.createDisplayPlaneSurfaceKHRUnique(create_info);
    }

    std::vector<vk::UniqueSurfaceKHR> create_surfaces(vk::Instance instance, vk::PhysicalDevice gpu, const std::vector<DirectConfig::Screen>& screens) {
        auto surfaces = std::vector<vk::UniqueSurfaceKHR>();

        auto displays = gpu.getDisplayPropertiesKHR();
        surfaces.reserve(screens.size());
        for (size_t i = 0; i < screens.size(); ++i) {
            if (screens[i].vulkan_index >= displays.size()) {
                throw std::runtime_error("Vulkan screen index out of range");
            }

            auto display_props = displays[screens[i].vulkan_index];
            surfaces.push_back(create_surface(instance, gpu, display_props.display));
        }

        return surfaces;
    }

    Device create_device(vk::PhysicalDevice gpu, const std::vector<vk::UniqueSurfaceKHR>& unique_surfaces) {
        if (!gpu_supports_extensions(gpu, DEVICE_EXTENSIONS)) {
            throw std::runtime_error("Gpu does not support required extensions");
        }

        auto surfaces = std::vector<vk::SurfaceKHR>();
        surfaces.reserve(unique_surfaces.size());
        for (auto& surface : unique_surfaces) {
            surfaces.push_back(surface.get());
        }

        if (auto queue = pick_graphics_queue(gpu, surfaces)) {
            return Device(gpu, DEVICE_EXTENSIONS, queue.value());
        } else {
            throw std::runtime_error("Gpu does not support graphics/present queue");
        }
    }
}

ScreenGroup::ScreenGroup(vk::Instance instance, vk::PhysicalDevice gpu, const std::vector<DirectConfig::Screen>& screens):
    surfaces(create_surfaces(instance, gpu, screens)),
    device(create_device(gpu, this->surfaces)) {

    this->screens.reserve(screens.size());
    for (size_t i = 0; i < screens.size(); ++i) {
        this->screens.emplace_back(this->device, this->surfaces[i].get(), screens[i].offset);
    }
}

ScreenGroup::~ScreenGroup() {
    if (this->device.logical)
        this->device.logical->waitIdle();
}
