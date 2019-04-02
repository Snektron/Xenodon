#ifndef _XENODON_PRESENT_SCREEN_H
#define _XENODON_PRESENT_SCREEN_H

#include <vector>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "graphics/Swapchain.h"
#include "math/Vec.h"

struct Output {
    virtual ~Output() = default;

    virtual uint32_t num_swap_images() const = 0;
    virtual SwapImage swap_image(uint32_t index) const = 0;
    virtual vk::Result present(Swapchain::PresentCallback f) = 0;

    virtual vk::Rect2D region() const = 0;
    virtual vk::AttachmentDescription color_attachment_descr() const = 0;
};

#endif
