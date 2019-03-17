#include "present/direct/DirectScreen.h"
#include <stdexcept>
#include <vector>

DirectScreen::DirectScreen(Device& device, vk::SurfaceKHR surface, vk::Offset2D offset):
    offset(offset),
    swapchain(device, surface, vk::Extent2D{0, 0}) {    
}

void DirectScreen::swap_buffers() {
    this->swapchain.swap_buffers();
}