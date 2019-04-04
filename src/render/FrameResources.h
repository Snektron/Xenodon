#ifndef _XENODON_RENDER_FRAMERESOURCES_H
#define _XENODON_RENDER_FRAMERESOURCES_H

#include <vector>
#include <vulkan/vulkan.hpp>
#include "graphics/core/Queue.h"
#include "backend/Output.h"
#include "backend/RenderDevice.h"

class FrameResources {
    Queue queue;
    Output* output;
    std::vector<vk::UniqueFramebuffer> framebuffers;
    std::vector<vk::UniqueCommandBuffer> command_buffers;

public:
    FrameResources(const RenderDevice& rendev, Output* output, vk::RenderPass pass);

    template <typename F>
    void submit(F f);
};

template <typename F>
void FrameResources::submit(F f) {
    uint32_t index = this->output->current_swap_index();
    const auto swap_image = this->output->swap_image(index);
    const auto framebuffer = this->framebuffers[index].get();
    const auto command_buffer = this->command_buffers[index].get();
    f(command_buffer, framebuffer);
    swap_image.submit(this->queue, command_buffer);
}

#endif
