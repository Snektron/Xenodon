#ifndef _XENODON_BACKEND_OUTPUT_H
#define _XENODON_BACKEND_OUTPUT_H

#include <vector>
#include <array>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "graphics/core/Queue.h"
#include "backend/SwapImage.h"

struct Output {
    virtual ~Output() = default;

    virtual uint32_t num_swap_images() const = 0;
    virtual uint32_t current_swap_index() const = 0;
    virtual SwapImage2 swap_image(uint32_t index) = 0;
    virtual vk::Rect2D region() const = 0;
    virtual vk::AttachmentDescription color_attachment_descr() const = 0;
};

#endif
