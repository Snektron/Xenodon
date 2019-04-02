#include "backend/xorg/XorgOutput.h"
#include <algorithm>
#include <array>
#include <vector>
#include <cstring>
#include "core/Error.h"
#include "core/Logger.h"
#include "backend/xorg/XorgWindow.h"

namespace {
    constexpr const std::array DEVICE_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    vk::UniqueSurfaceKHR create_surface(Instance& instance, XorgWindow& window) {
        auto [connection, xid] = window.x_handles();
        return instance->createXcbSurfaceKHRUnique({
            {},
            connection,
            xid
        });
    }

    Device create_device(Instance& instance, vk::SurfaceKHR surface) {
        const auto& physdevs = instance.physical_devices();
        for (auto type : {vk::PhysicalDeviceType::eDiscreteGpu, vk::PhysicalDeviceType::eIntegratedGpu}) {
            for (size_t i = 0; i < physdevs.size(); ++i) {
                const auto& physdev = physdevs[i];
                if (physdev.type() != type ||
                    !physdev.supports_extensions(DEVICE_EXTENSIONS) ||
                    !physdev.supports_surface(surface)) {
                    continue;
                }

                if (auto family = physdev.find_queue_family(vk::QueueFlagBits::eGraphics, surface)) {
                    LOGGER.log("Picked GPU {}: '{}'", i, physdev.name());
                    LOGGER.log("Graphics queue family: {}", family.value());
                    return Device(physdev.get(), DEVICE_EXTENSIONS, family.value());
                }
            }
        }

        throw Error("Failed to find suitable physical device");
    }
}

XorgOutput::XorgOutput(Instance& instance, EventDispatcher& dispatcher, vk::Extent2D extent):
    window(dispatcher, extent),
    surface(create_surface(instance, this->window)),
    device(create_device(instance, this->surface.get())),
    swapchain(this->device, this->surface.get(), extent) {
    this->log();
}

XorgOutput::XorgOutput(Instance& instance, EventDispatcher& dispatcher, const XorgMultiGpuConfig::Output& config):
    window(dispatcher, config.displayname.c_str(), true),
    surface(create_surface(instance, this->window)),
    device(create_device(instance, this->surface.get())),
    swapchain(this->device, this->surface.get(), this->window.extent()) {
    this->log();
}

XorgOutput::~XorgOutput() {
    this->device.logical->waitIdle();
}

void XorgOutput::poll_events() {
    this->window.poll_events([this](vk::Extent2D new_extent) {
        this->device.logical->waitIdle();
        this->swapchain.recreate(new_extent);
    });
}

uint32_t XorgOutput::num_swap_images() const {
    return this->swapchain.num_images();
}

SwapImage XorgOutput::swap_image(uint32_t index) const {
    return this->swapchain.image(index);
}

vk::Result XorgOutput::present(Swapchain::PresentCallback f) {
    return this->swapchain.present(f);
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