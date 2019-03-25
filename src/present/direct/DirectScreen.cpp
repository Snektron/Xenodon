#include "present/direct/DirectScreen.h"
#include <stdexcept>
#include <vector>

DirectScreen::DirectScreen(Device& device, vk::SurfaceKHR surface, vk::Offset2D offset):
    offset(offset),
    swapchain(device, surface, vk::Extent2D{0, 0}) {    
}

uint32_t DirectScreen::num_swap_images() const {
    return this->swapchain.num_images();
}

SwapImage DirectScreen::swap_image(uint32_t index) const {
    return this->swapchain.image(index);
}

vk::Result DirectScreen::present(Swapchain::PresentCallback f) {
    return this->swapchain.present(f);
}

vk::Rect2D DirectScreen::region() const {
    return {this->offset, this->swapchain.surface_extent()};
}

vk::AttachmentDescription DirectScreen::color_attachment_descr() const {
    auto descr = vk::AttachmentDescription();
    descr.format = this->swapchain.surface_format().format;
    descr.loadOp = vk::AttachmentLoadOp::eClear;
    descr.finalLayout = vk::ImageLayout::ePresentSrcKHR;
    return descr;
}
