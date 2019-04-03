#ifndef _XENODON_BACKEND_SWAPIMAGE_H
#define _XENODON_BACKEND_SWAPIMAGE_H

#include <vulkan/vulkan.hpp>
#include "graphics/core/Queue.h"
#include "graphics/core/Swapchain.h"

struct SwapImage {
    vk::Image image;
    vk::ImageView view;
    vk::Semaphore image_acquired;
    vk::Semaphore render_finished;
    vk::Fence frame_fence;

    SwapImage(const Swapchain::SwapImageResources& resources);
    SwapImage(vk::Image image, vk::ImageView view, vk::Fence frame_fence = vk::Fence());
    void submit(Queue queue, vk::CommandBuffer cmd_buf, vk::PipelineStageFlags flags = vk::PipelineStageFlagBits::eColorAttachmentOutput) const;
};

#endif
