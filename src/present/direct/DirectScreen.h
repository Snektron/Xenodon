#ifndef _XENODON_PRESENT_DIRECT_DIRECTSCREEN_H
#define _XENODON_PRESENT_DIRECT_DIRECTSCREEN_H

#include <cstdint>
#include <vulkan/vulkan.hpp>
#include "graphics/Device.h"
#include "graphics/Swapchain.h"
#include "present/Screen.h"

class DirectScreen final: public Screen {
    vk::Offset2D offset;
    Swapchain swapchain;

public:
    DirectScreen(Device& device, vk::SurfaceKHR surface, vk::Offset2D offset);
    void swap_buffers();

    uint32_t active_index() const override;
    SwapImage swap_image(uint32_t index) const override;
    vk::Rect2D region() const override;
};

#endif
