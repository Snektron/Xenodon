#ifndef _XENODON_PRESENT_FRAME_H
#define _XENODON_PRESENT_FRAME_H

#include <cstddef>
#include <vulkan/vulkan.hpp>

struct Frame {
    uint32_t index;
    vk::Semaphore image_acquired;
    vk::Semaphore render_finished;
    vk::Image image;
    vk::ImageView image_view;
    vk::CommandBuffer command_buffer;
};

#endif
