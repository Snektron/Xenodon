#include "present/direct/DirectScreen.h"
#include <stdexcept>
#include <vector>
#include <iostream>

DirectScreen::DirectScreen(Device& device, vk::SurfaceKHR surface, vk::Offset2D offset):
    offset(offset),
    swapchain(device, surface, vk::Extent2D{0, 0}) {    
}

void DirectScreen::swap_buffers() {
    vk::Result res = this->swapchain.swap_buffers();
    if (res != vk::Result::eSuccess) {
        std::cout << "Failed to swap; frame dropped" << std::endl;
    }
}

uint32_t DirectScreen::active_index() const {
    return this->swapchain.current_index();
}

uint32_t DirectScreen::num_swap_images() const {
    return this->swapchain.num_images();
}

SwapImage DirectScreen::swap_image(uint32_t index) const {
    const auto& image = this->swapchain.image(index);

    return {
        image.image,
        image.image_view.get(),
        image.command_buffer.get()
    };
}

vk::Rect2D DirectScreen::region() const {
    return {this->offset, this->swapchain.surface_extent()};
}

vk::AttachmentDescription DirectScreen::color_attachment_descr() const {
    auto descr = vk::AttachmentDescription();
    descr.format = this->swapchain.surface_format();
    descr.loadOp = vk::AttachmentLoadOp::eClear;
    descr.finalLayout = vk::ImageLayout::ePresentSrcKHR;
    return descr;
}
