#include "backend/direct/DirectOutput.h"
#include <vector>
#include "core/Logger.h"

DirectOutput::DirectOutput(Device& device, Queue& graphics_queue, vk::SurfaceKHR surface, vk::Offset2D offset):
    offset(offset),
    swapchain(device, graphics_queue, surface, vk::Extent2D{0, 0}) {
}

void DirectOutput::swap_buffers() {
    this->swapchain.swap_buffers();
}

uint32_t DirectOutput::num_swap_images() const {
    return this->swapchain.num_images();
}

uint32_t DirectOutput::current_swap_index() const {
    return this->swapchain.current_index();
}

SwapImage DirectOutput::swap_image(uint32_t index) {
    return SwapImage(this->swapchain.image(index));
}

vk::Rect2D DirectOutput::region() const {
    return {this->offset, this->swapchain.surface_extent()};
}

vk::AttachmentDescription DirectOutput::color_attachment_descr() const {
    auto descr = vk::AttachmentDescription();
    descr.format = this->swapchain.surface_format().format;
    descr.loadOp = vk::AttachmentLoadOp::eClear;
    descr.finalLayout = vk::ImageLayout::ePresentSrcKHR;
    return descr;
}

void DirectOutput::log() const {
    LOGGER.log("\t\tPresent mode: {}", vk::to_string(this->swapchain.surface_present_mode()));
    auto extent = this->swapchain.surface_extent();
    LOGGER.log("\t\tExtent: {}x{}", extent.width, extent.height);
    auto surface_format = this->swapchain.surface_format();
    LOGGER.log("\t\tFormat: {}", vk::to_string(surface_format.format));
    LOGGER.log("\t\tColorspace: {}", vk::to_string(surface_format.colorSpace));
    LOGGER.log("\t\tSwapchain images: {}", this->swapchain.num_images());
}