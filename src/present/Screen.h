#ifndef _XENODON_PRESENT_SCREEN_H
#define _XENODON_PRESENT_SCREEN_H

#include <vector>
#include <cstddef>
#include <vulkan/vulkan.hpp>

struct SwapImage {
    vk::Image image;
    vk::ImageView view;
    vk::CommandBuffer command_buffer;
};

struct Screen {
    virtual ~Screen() = default;

    SwapImage active_image() const {
        return this->swap_image(this->active_index());
    }

    virtual uint32_t active_index() const = 0;
    virtual uint32_t num_swap_images() const = 0;
    virtual SwapImage swap_image(uint32_t index) const = 0;
    virtual vk::Rect2D region() const = 0;
    virtual vk::AttachmentDescription color_attachment_descr() const = 0;
    virtual void swap_buffers() = 0;
};

#endif
