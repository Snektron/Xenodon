#include "render/FrameResources.h"

FrameResources::FrameResources(const RenderDevice& rendev, Output* output, vk::RenderPass pass):
    queue(rendev.graphics_queue),
    output(output) {

    uint32_t images = this->output->num_swap_images();
    this->command_buffers = rendev.graphics_command_pool.allocate_command_buffers(images);

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
