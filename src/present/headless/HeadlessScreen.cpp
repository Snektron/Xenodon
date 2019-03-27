#include "present/headless/HeadlessScreen.h"
#include "graphics/utility.h"

namespace {
    Device create_device(vk::PhysicalDevice gpu) {
        if (auto queue = pick_graphics_queue(gpu, nullptr)) {
            return Device(gpu, nullptr, queue.value());
        } else {
            throw std::runtime_error("Gpu does not support graphics/present queue");
        }
    }
}

HeadlessScreen::HeadlessScreen(vk::PhysicalDevice gpu, vk::Rect2D render_region):
    render_region(render_region),
    device(create_device(gpu)) {
}

uint32_t HeadlessScreen::num_swap_images() const {
    return 1;
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