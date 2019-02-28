#include "present/xorg/XorgDisplay.h"
#include <stdexcept>
#include <utility>

XorgDisplay::XorgDisplay(vk::Instance instance, EventDispatcher& dispatcher, uint16_t width, uint16_t height):
    window(dispatcher, width, height),
    surface(instance, this->window) {
}

vk::Extent2D XorgDisplay::size() {
    return vk::Extent2D{
        static_cast<uint32_t>(this->window.width),
        static_cast<uint32_t>(this->window.height)
    };
}

void XorgDisplay::poll_events() {
    this->window.poll_events(this->surface);
}
