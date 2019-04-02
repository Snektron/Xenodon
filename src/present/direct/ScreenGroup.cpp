#include "present/direct/ScreenGroup.h"
#include <array>

namespace {
    constexpr const std::array DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_DISPLAY_CONTROL_EXTENSION_NAME
    };

    vk::UniqueSurfaceKHR create_surface(Instance& instance, const PhysicalDevice& gpu, vk::DisplayKHR display) {
        vk::DisplayModePropertiesKHR mode_props;
        uint32_t property_count = 1;

        vk::createResultValue(
            gpu->getDisplayModePropertiesKHR(
                display,
                &property_count,
                &mode_props
            ),
            __PRETTY_FUNCTION__,
            {vk::Result::eSuccess, vk::Result::eIncomplete}
        );

        auto [plane_index, stack_index] = gpu.find_display_plane(display).value();

        auto create_info = vk::DisplaySurfaceCreateInfoKHR();
        create_info.flags = {};
        create_info.displayMode = mode_props.displayMode;
        create_info.planeIndex = plane_index;
        create_info.planeStackIndex = stack_index;
        create_info.transform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
        create_info.alphaMode = vk::DisplayPlaneAlphaFlagBitsKHR::eOpaque;
        create_info.globalAlpha = 1.0f;
        create_info.imageExtent = mode_props.parameters.visibleRegion;

        return instance->createDisplayPlaneSurfaceKHRUnique(create_info);
    }

    std::vector<vk::UniqueSurfaceKHR> create_surfaces(Instance& instance, const PhysicalDevice& gpu, const std::vector<DirectConfig::Screen>& screens) {
        auto surfaces = std::vector<vk::UniqueSurfaceKHR>();

        auto displays = gpu->getDisplayPropertiesKHR();
        surfaces.reserve(screens.size());
        for (size_t i = 0; i < screens.size(); ++i) {
            if (screens[i].vulkan_index >= displays.size()) {
                throw Error("Vulkan screen index {} is out of range", i);
            }

            auto display_props = displays[screens[i].vulkan_index];
            surfaces.push_back(create_surface(instance, gpu, display_props.display));
        }

        return surfaces;
    }

    Device create_device(const PhysicalDevice& gpu, const std::vector<vk::UniqueSurfaceKHR>& unique_surfaces) {
        if (!gpu.supports_extensions(DEVICE_EXTENSIONS)) {
            throw Error("Gpu does not support required extensions");
        }

        auto surfaces = std::vector<vk::SurfaceKHR>();
        surfaces.reserve(unique_surfaces.size());
        for (auto& surface : unique_surfaces) {
            surfaces.push_back(surface.get());
        }

        if (auto queue = gpu.find_queue_family(vk::QueueFlagBits::eGraphics, surfaces)) {
            return Device(gpu.get(), DEVICE_EXTENSIONS, queue.value());
        } else {
            throw Error("Gpu does not support graphics/present queue");
        }
    }
}

ScreenGroup::ScreenGroup(Instance& instance, const PhysicalDevice& gpu, const std::vector<DirectConfig::Screen>& screens):
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
