#include "render/FrameResources.h"

FrameResources::FrameResources(const RenderDevice& rendev, Output* output, vk::RenderPass pass):
    queue(rendev.graphics_queue),
    output(output) {

    uint32_t images = this->output->num_swap_images();
    auto command_buffers_info = vk::CommandBufferAllocateInfo(rendev.graphics_pool());
    command_buffers_info.commandBufferCount = static_cast<uint32_t>(images);
    this->command_buffers = rendev.device->allocateCommandBuffersUnique(command_buffers_info);

    this->framebuffers.reserve(images);
    auto extent = output->region().extent;
    for (uint32_t i = 0; i < images; ++i) {
        auto swap_image = this->output->swap_image(i);

        auto create_info = vk::FramebufferCreateInfo(
            {},
            pass,
            1,
            &swap_image.view,
            extent.width,
            extent.height,
            1
        );

        this->framebuffers.push_back(rendev.device->createFramebufferUnique(create_info));
    }
}
