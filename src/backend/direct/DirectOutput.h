#ifndef _XENODON_BACKEND_DIRECT_DIRECTOUTPUT_H
#define _XENODON_BACKEND_DIRECT_DIRECTOUTPUT_H

#include <cstdint>
#include <vulkan/vulkan.hpp>
#include "graphics/core/Device.h"
#include "graphics/core/Swapchain.h"
#include "graphics/core/Queue.h"
#include "backend/Output.h"

class DirectOutput final: public Output {
    vk::Offset2D offset;
    Swapchain swapchain;

public:
    DirectOutput(Device& device, Queue& graphics_queue, vk::SurfaceKHR surface, vk::Offset2D offset);

    void swap_buffers();
    void log() const;

    uint32_t num_swap_images() const override;
    uint32_t current_swap_index() const override;
    SwapImage swap_image(uint32_t index) override;
    vk::Rect2D region() const override;
    vk::AttachmentDescription color_attachment_descr() const override;
};

#endif
