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

    uint32_t num_swap_images() const override;
    SwapImage swap_image(uint32_t index) const override;
    vk::Result present(Swapchain::PresentCallback f) override;

    vk::Rect2D region() const override;
    vk::AttachmentDescription color_attachment_descr() const override;

    friend class DirectDisplay;
};

#endif
