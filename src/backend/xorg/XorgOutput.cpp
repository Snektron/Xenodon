#include "backend/xorg/XorgOutput.h"
#include <algorithm>
#include <array>
#include <vector>
#include <cstring>
#include "core/Error.h"
#include "core/Logger.h"

namespace {
    constexpr const std::array DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    vk::UniqueSurfaceKHR create_surface(Instance& instance, Window& window) {
        auto [connection, xid] = window.x_handles();
        return instance->createXcbSurfaceKHRUnique({
            {},
            connection,
            xid
        });
    }

    RenderDevice create_render_device(Instance& instance, vk::SurfaceKHR surface) {
        const auto& physdevs = instance.physical_devices();
        for (auto type : {vk::PhysicalDeviceType::eDiscreteGpu, vk::PhysicalDeviceType::eIntegratedGpu}) {
            for (size_t i = 0; i < physdevs.size(); ++i) {
                const auto& physdev = physdevs[i];
                if (physdev.type() != type ||
                    !physdev.supports_extensions(DEVICE_EXTENSIONS) ||
                    !physdev.supports_surface(surface)) {
                    continue;
                }

                const auto queue_families = physdev.find_queues(surface);

                if (queue_families) {
                    LOGGER.log("Picked GPU {}: '{}'", i, physdev.name());
                    LOGGER.log("Graphics queue family: {}", queue_families.value().graphics_family);
                    LOGGER.log("Compute queue family: {}", queue_families.value().compute_family);

                    const auto families = std::array {
                        queue_families.value().graphics_family,
                        queue_families.value().compute_family
                    };

                    return RenderDevice(
                        Device(physdev, families, DEVICE_EXTENSIONS),
                        queue_families.value().graphics_family,
                        queue_families.value().compute_family,
                        1,
                        physdev.properties().limits.timestampPeriod
                    );
                }
            }
        }

        throw Error("Failed to find suitable physical device");
    }
}

XorgOutput::XorgOutput(Instance& instance, EventDispatcher& dispatcher, vk::Extent2D extent):
    window(dispatcher, extent),
    surface(create_surface(instance, this->window)),
    rendev(create_render_device(instance, this->surface.get())),
    swapchain(this->rendev.device, this->rendev.graphics_queue, this->surface.get(), this->window.extent()) {
    this->log();
}

XorgOutput::XorgOutput(Instance& instance, EventDispatcher& dispatcher, const XorgMultiGpuConfig::Output& config):
    window(dispatcher, config.displayname.c_str(), true),
    surface(create_surface(instance, this->window)),
    rendev(create_render_device(instance, this->surface.get())),
    swapchain(this->rendev.device, this->rendev.graphics_queue, this->surface.get(), this->window.extent()) {
    this->log();
}

XorgOutput::~XorgOutput() {
    this->rendev.device->waitIdle();
}

void XorgOutput::poll_events() {
    this->window.poll_events([this](vk::Extent2D new_extent) {
        this->rendev.device->waitIdle();
        this->swapchain.recreate(new_extent);
    });
}

uint32_t XorgOutput::num_swap_images() const {
    return this->swapchain.num_images();
}

uint32_t XorgOutput::current_swap_index() const {
    return this->swapchain.current_index();
}

SwapImage XorgOutput::swap_image(uint32_t index) {
    return SwapImage(this->swapchain.image(index));
}

void XorgOutput::swap_buffers() {
    auto res = this->swapchain.swap_buffers();
    if (res != vk::Result::eSuccess) {
        LOGGER.log("Failed to swap image: {}", vk::to_string(res));
    }
}

vk::Rect2D XorgOutput::region() const {
    return {{0, 0}, this->swapchain.surface_extent()};
}

vk::AttachmentDescription XorgOutput::color_attachment_descr() const {
    auto descr = vk::AttachmentDescription();
    descr.format = this->swapchain.surface_format().format;
    descr.loadOp = vk::AttachmentLoadOp::eClear;
    descr.finalLayout = vk::ImageLayout::ePresentSrcKHR;
    return descr;
}

void XorgOutput::log() const {
    LOGGER.log("Output info:");
    LOGGER.log("\tPresent mode: {}", vk::to_string(this->swapchain.surface_present_mode()));
    auto surface_format = this->swapchain.surface_format();
    LOGGER.log("\tFormat: {}", vk::to_string(surface_format.format));
    LOGGER.log("\tColorspace: {}", vk::to_string(surface_format.colorSpace));
    auto swap_extent = this->swapchain.surface_extent();
    LOGGER.log("\tExtent: {}x{}", swap_extent.width, swap_extent.height);
    LOGGER.log("\tSwapchain images: {}", this->swapchain.num_images());
}