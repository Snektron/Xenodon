#include "present/headless/HeadlessScreen.h"

HeadlessScreen::HeadlessScreen(vk::PhysicalDevice gpu, vk::Rect2D render_region):
    render_region(render_region) {

}

uint32_t HeadlessScreen::num_swap_images() const {

}

SwapImage HeadlessScreen::swap_image(uint32_t index) const {

}

vk::Result HeadlessScreen::present(Swapchain::PresentCallback f) {

}

vk::Rect2D HeadlessScreen::region() const {
    return this->render_region;
}

vk::AttachmentDescription HeadlessScreen::color_attachment_descr() const {

}