#include "render/DeviceRenderer.h"

RenderOutput::RenderOutput(const RenderDevice& rendev, Output* output):
    rendev(rendev),
    output(output),
    region(output->region()),
    renderer(std::make_unique<TestRenderer>(rendev.device, this->region, output->color_attachment_descr())) {
    uint32_t images = this->output->num_swap_images();

    auto command_buffers_info = vk::CommandBufferAllocateInfo(rendev.graphics_pool());
    command_buffers_info.commandBufferCount = static_cast<uint32_t>(images);
    this->command_buffers = rendev.device->allocateCommandBuffersUnique(command_buffers_info);

    this->framebuffers.reserve(images);
    for (uint32_t i = 0; i < images; ++i) {
        auto swap_image = this->output->swap_image(i);

        auto create_info = vk::FramebufferCreateInfo(
            {},
            this->renderer->final_render_pass(),
            1,
            &swap_image.view,
            this->region.extent.width,
            this->region.extent.height,
            1
        );

        this->framebuffers.push_back(rendev.device->createFramebufferUnique(create_info));
    }
}

void RenderOutput::render() {
    uint32_t index = this->output->current_swap_index();
    const auto swap_image = this->output->swap_image(index);
    const auto framebuffer = this->framebuffers[index].get();
    const auto command_buffer = this->command_buffers[index].get();
    this->renderer->present(command_buffer, framebuffer);
    swap_image.submit(this->rendev.graphics_queue, command_buffer);
}

DeviceRenderer::DeviceRenderer(Display* display, size_t device):
    rendev(display->render_device(device)) {

    this->outputs.reserve(this->rendev.outputs);
    for (size_t i = 0; i < this->rendev.outputs; ++i) {
        this->outputs.push_back(std::make_unique<RenderOutput>(this->rendev, display->output(device, i)));
    }
}

void DeviceRenderer::render() {
    for (auto& output : this->outputs) {
        output->render();
    }
}

void DeviceRenderer::recreate(size_t output) {
    this->outputs[output] = std::make_unique<RenderOutput>(this->rendev, this->outputs[output]->output);
}

DeviceRenderer::~DeviceRenderer() {
    this->rendev.device->waitIdle();
}