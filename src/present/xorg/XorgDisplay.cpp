#include "present/xorg/XorgDisplay.h"
#include <stdexcept>
#include <utility>

XorgDisplay::XorgDisplay(vk::Instance instance, EventDispatcher& dispatcher, uint16_t width, uint16_t height):
    instance(instance),
    window(dispatcher, width, height) {
}

vk::Extent2D XorgDisplay::size() {
    return vk::Extent2D{
        static_cast<uint32_t>(this->window.width),
        static_cast<uint32_t>(this->window.height)
    };
}

void XorgDisplay::poll_events() {
    this->window.poll_events();
}
