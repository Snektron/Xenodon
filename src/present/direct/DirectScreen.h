#ifndef _XENODON_PRESENT_DIRECT_DIRECTSCREEN_H
#define _XENODON_PRESENT_DIRECT_DIRECTSCREEN_H

#include <cstdint>
#include <vulkan/vulkan.hpp>
#include "graphics/Device.h"
#include "graphics/Swapchain.h"
#include "present/Screen.h"
#include "present/direct/DisplayConfig.h"

class DirectScreen final: public Screen {
    vk::Offset2D offset;
    Swapchain swapchain;

public:
    DirectScreen(Device& device, vk::SurfaceKHR surface, vk::Offset2D offset);
    void swap_buffers();
};

#endif
