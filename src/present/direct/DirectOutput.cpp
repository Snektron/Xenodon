#include "present/direct/DirectOutput.h"
#include <stdexcept>
#include <vector>

DirectOutput::DirectOutput(Device& device, vk::SurfaceKHR surface, vk::Offset2D offset):
    offset(offset),
    swapchain(device, surface, vk::Extent2D{0, 0}) {
}

uint32_t DirectOutput::num_swap_images() const {
    return this->swapchain.num_images();
}

SwapImage DirectOutput::swap_image(uint32_t index) const {
    return this->swapchain.image(index);
}

vk::Result DirectOutput::present(Swapchain::PresentCallback f) {
    return this->swapchain.present(f);
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
