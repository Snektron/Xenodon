#include "present/xorg/XorgScreen.h"
#include <stdexcept>
#include <algorithm>
#include <array>
#include <vector>
#include <cstring>
#include "core/Logger.h"
#include "present/xorg/XorgWindow.h"
#include "graphics/utility.h"

namespace {
    constexpr const std::array DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    struct GpuInfo {
        vk::PhysicalDevice gpu;
        const char* device_name;
        uint32_t graphics_queue_index;
        uint32_t index;
    };

    GpuInfo pick_gpu(vk::Instance instance, vk::SurfaceKHR surface) {
        auto gpus = instance.enumeratePhysicalDevices();
        auto gpu_infos = std::vector<std::pair<vk::PhysicalDevice, vk::PhysicalDeviceProperties>>(gpus.size());

        std::transform(gpus.begin(), gpus.end(), gpu_infos.begin(), [](vk::PhysicalDevice gpu) {
            return std::pair {
                gpu,
                gpu.getProperties()
            };
        });

        // Make sure discrete GPUs are appearing before integrated GPUs, because
        // we always want to chose discrete over integrated.
        std::sort(gpu_infos.begin(), gpu_infos.end(), [](const auto& l, const auto& r) {
            auto rate = [](const auto& gpu) {
                switch (gpu.second.deviceType) {
                    case vk::PhysicalDeviceType::eDiscreteGpu:
                        return 0;
                    case vk::PhysicalDeviceType::eIntegratedGpu:
                        return 1;
                    default:
                        return 2;
                }
            };

            return rate(l) < rate(r);
        });

        for (size_t i = 0; i < gpu_infos.size(); ++i) {
            auto& [gpu, props] = gpu_infos[i];

            // Because we sorted earlier, if the type isnt one of these there arent
            // any more candidates, so we can exit early
            if (props.deviceType != vk::PhysicalDeviceType::eDiscreteGpu &&
                props.deviceType != vk::PhysicalDeviceType::eIntegratedGpu)
                break;

            if (!gpu_supports_extensions(gpu, DEVICE_EXTENSIONS)
                || !gpu_supports_surface(gpu, surface))
                continue;

            if (auto queue = pick_graphics_queue(gpu, surface)) {
                return {gpu, props.deviceName, queue.value(), static_cast<uint32_t>(i)};
            }
        }

        throw std::runtime_error("Failed to find a suitable physical device");
    }

    vk::UniqueSurfaceKHR create_surface(vk::Instance instance, XorgWindow& window) {
        auto [connection, xid] = window.x_handles();
        auto create_info = vk::XcbSurfaceCreateInfoKHR(
            {},
            connection,
            xid
        );

        return instance.createXcbSurfaceKHRUnique(create_info);
    }

    Device create_device(vk::Instance instance, vk::SurfaceKHR surface) {
        auto [gpu, name, gqi, index] = pick_gpu(instance, surface);
        LOGGER.log("Picked GPU {}: '{}'", index, name);
        LOGGER.log("Graphics queue index: {}", gqi);
        return Device(gpu, DEVICE_EXTENSIONS, gqi);
    }
}

XorgScreen::XorgScreen(vk::Instance instance, XorgWindow& window, vk::Extent2D window_extent):
    surface(create_surface(instance, window)),
    device(create_device(instance, this->surface.get())),
    swapchain(this->device, this->surface.get(), window_extent) {
    LOGGER.log("Screen info:");
    LOGGER.log("\tPresent mode: {}", vk::to_string(this->swapchain.surface_present_mode()));
    auto extent = this->swapchain.surface_extent();
    LOGGER.log("\tExtent: {}x{}", extent.width, extent.height);
    auto surface_format = this->swapchain.surface_format();
    LOGGER.log("\tFormat: {}", vk::to_string(surface_format.format));
    LOGGER.log("\tColorspace: {}", vk::to_string(surface_format.colorSpace));
    LOGGER.log("\tSwapchain images: {}", this->swapchain.num_images());
}

XorgScreen::~XorgScreen() {
    this->device.logical->waitIdle();
}

void XorgScreen::resize(vk::Extent2D window_extent) {
    this->device.logical->waitIdle();
    this->swapchain.recreate(window_extent);
}

uint32_t XorgScreen::num_swap_images() const {
    return this->swapchain.num_images();
}

SwapImage XorgScreen::swap_image(uint32_t index) const {
    return this->swapchain.image(index);
}

vk::Result XorgScreen::present(Swapchain::PresentCallback f) {
    return this->swapchain.present(f);
}

vk::Rect2D XorgScreen::region() const {
    return {{0, 0}, this->swapchain.surface_extent()};
}

vk::AttachmentDescription XorgScreen::color_attachment_descr() const {
    auto descr = vk::AttachmentDescription();
    descr.format = this->swapchain.surface_format().format;
    descr.loadOp = vk::AttachmentLoadOp::eClear;
    descr.finalLayout = vk::ImageLayout::ePresentSrcKHR;
    return descr;
}
