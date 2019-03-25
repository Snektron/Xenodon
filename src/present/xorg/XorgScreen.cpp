#include "present/xorg/XorgScreen.h"
#include <stdexcept>
#include <algorithm>
#include <array>
#include <vector>
#include <cstring>
#include "core/Logger.h"
#include "present/xorg/XorgWindow.h"
#include "graphics/support.h"

namespace {
    constexpr const std::array DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    struct GpuInfo {
        vk::PhysicalDevice gpu;
        const char* device_name;
        uint32_t graphics_queue_index;
        uint32_t present_queue_index;
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

        auto graphics_supported = std::vector<uint32_t>();
        auto present_supported = std::vector<uint32_t>();
        for (size_t i = 0; i < gpu_infos.size(); ++i) {
            auto& [gpu, props] = gpu_infos[i];

            // Because we sorted earlier, if the type isnt one of these there arent
            // any more candidates, so we can exit early
            if (props.deviceType != vk::PhysicalDeviceType::eDiscreteGpu &&
                props.deviceType != vk::PhysicalDeviceType::eIntegratedGpu)
                break;

            if (!check_extension_support(gpu, DEVICE_EXTENSIONS)
                || !check_surface_support(gpu, surface))
                continue;

            // Find a compute and present queue for of the gpu
            {
                graphics_supported.clear();
                present_supported.clear();
                auto queue_families = gpu.getQueueFamilyProperties();

                uint32_t num_queues = static_cast<uint32_t>(queue_families.size());
                for (uint32_t i = 0; i < num_queues; ++i) {
                    if (queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics)
                        graphics_supported.push_back(i);

                    if (gpu.getSurfaceSupportKHR(i, surface))
                        present_supported.push_back(i);
                }

                if (graphics_supported.empty() || present_supported.empty())
                    continue;

                auto git = graphics_supported.begin();
                auto pit = present_supported.begin();

                // Check if theres a queue with both supported
                while (git != graphics_supported.end() && pit != present_supported.end()) {
                    uint32_t g = *git;
                    uint32_t p = *pit;

                    if (g == p)
                        return {gpu, props.deviceName, g, p, static_cast<uint32_t>(i)};
                    else if (g < p)
                        ++git;
                    else
                        ++pit;
                }

                // No queue with both supported, but both are supported, so just take the first of both
                return {gpu, props.deviceName, graphics_supported[0], present_supported[0], static_cast<uint32_t>(i)};
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
        auto [gpu, name, gqi, pqi, index] = pick_gpu(instance, surface);
        LOGGER.log("Picked GPU {}: '{}'", index, name);
        return Device(gpu, DEVICE_EXTENSIONS, gqi, pqi);
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
