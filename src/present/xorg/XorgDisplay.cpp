#include "present/xorg/XorgDisplay.h"
#include <stdexcept>
#include <utility>

XorgDisplay::XorgDisplay(vk::Instance instance, EventDispatcher& dispatcher, uint16_t width, uint16_t height):
    window(dispatcher, width, height),
    screen(instance, this->window, {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}) {
}

vk::Extent2D XorgDisplay::size() const {
    return this->screen.size();
}

void XorgDisplay::poll_events() {
    this->window.poll_events(this->screen);
}

void XorgDisplay::swap_buffers() {
    this->screen.swap_buffers();
}
