#include "present/xorg/XorgDisplay.h"
#include <stdexcept>
#include <utility>

XorgDisplay::XorgDisplay(vk::Instance instance, EventDispatcher& dispatcher, uint16_t width, uint16_t height):
    window(dispatcher, width, height),
    screen(instance, this->window, {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}) {
}

Setup XorgDisplay::setup() {
    return {1};
}

Device& XorgDisplay::device_at(size_t gpu_index) {
    // gpu_index should be 0.
    return this->screen.device;
}

Screen* XorgDisplay::screen_at(size_t gpu_index, size_t screen_index) {
    // gpu_index and screen_index should be 0.
    return &this->screen;
}

void XorgDisplay::poll_events() {
    this->window.poll_events(this->screen);
}

void XorgDisplay::swap_buffers() {
    this->screen.swap_buffers();
}
