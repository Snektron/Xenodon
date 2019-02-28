#include "present/xorg/XorgDisplay.h"
#include <stdexcept>
#include <utility>

XorgDisplay::XorgDisplay(vk::Instance instance, EventDispatcher& dispatcher, uint16_t width, uint16_t height):
    window(dispatcher, width, height),
    screen(instance, this->window) {
}

vk::Extent2D XorgDisplay::size() const {
    return this->screen.extent;
}

void XorgDisplay::poll_events() {
    this->window.poll_events(this->screen);
}
