#include "backend/direct/ScreenGroup.h"
#include <array>
#include "core/Logger.h"

namespace {
    constexpr const std::array DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_DISPLAY_CONTROL_EXTENSION_NAME
    };

    vk::UniqueSurfaceKHR create_surface(Instance& instance, const PhysicalDevice& physdev, vk::DisplayKHR display) {
        vk::DisplayModePropertiesKHR mode_props;
        uint32_t property_count = 1;

        vk::createResultValue(
            physdev->getDisplayModePropertiesKHR(
                display,
                &property_count,
                &mode_props
            ),
            __PRETTY_FUNCTION__,
            {vk::Result::eSuccess, vk::Result::eIncomplete}
        );

        auto opt = physdev.find_display_plane(display);
        if (!opt) {
            throw Error("Failed to find display plane");
        }

        auto [plane_index, stack_index] = opt.value();

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

    std::vector<vk::UniqueSurfaceKHR> create_surfaces(Instance& instance, const PhysicalDevice& physdev, const std::vector<DirectConfig::Output>& outputs) {
        auto surfaces = std::vector<vk::UniqueSurfaceKHR>();

        auto displays = physdev->getDisplayPropertiesKHR();
        surfaces.reserve(outputs.size());
        for (size_t i = 0; i < outputs.size(); ++i) {
            if (outputs[i].vulkan_index >= displays.size()) {
                throw Error("Vulkan output index {} is out of range", i);
            }

            auto display_props = displays[outputs[i].vulkan_index];
            surfaces.push_back(create_surface(instance, physdev, display_props.display));
        }

        return surfaces;
    }

    RenderDevice create_render_device(const PhysicalDevice& physdev, const std::vector<vk::UniqueSurfaceKHR>& unique_surfaces) {
        if (!physdev.supports_extensions(DEVICE_EXTENSIONS)) {
            throw Error("Gpu does not support required extensions");
        }

        auto surfaces = std::vector<vk::SurfaceKHR>();
        surfaces.reserve(unique_surfaces.size());
        for (auto& surface : unique_surfaces) {
            if (!physdev.supports_surface(surface.get())) {
                throw Error("Gpu doesnt support surface");
            }

            surfaces.push_back(surface.get());
        }

        if (auto family = physdev.find_queue_family(vk::QueueFlagBits::eGraphics, surfaces)) {
            return RenderDevice(
                Device(physdev, family.value(), DEVICE_EXTENSIONS),
                family.value(),
                surfaces.size()
            );
        } else {
            throw Error("Gpu does not support graphics/present queue");
        }
    }
}

ScreenGroup::ScreenGroup(Instance& instance, const PhysicalDevice& physdev, const std::vector<DirectConfig::Output>& outputs):
    surfaces(create_surfaces(instance, physdev, outputs)),
    rendev(create_render_device(physdev, this->surfaces)) {

    this->outputs.reserve(outputs.size());
    for (size_t i = 0; i < outputs.size(); ++i) {
        this->outputs.emplace_back(this->rendev.device, this->rendev.graphics_queue, this->surfaces[i].get(), outputs[i].offset);
    }
}

ScreenGroup::~ScreenGroup() {
    this->rendev.device->waitIdle();
}

void ScreenGroup::swap_buffers() {
    for (auto& output : this->outputs) {
        output.swap_buffers();
    }
}

void ScreenGroup::log() const {
    for (size_t i = 0; i < this->outputs.size(); ++i) {
        LOGGER.log("\tOutput {}:", i);
    }
}