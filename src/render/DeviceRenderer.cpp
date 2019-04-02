#include "render/DeviceRenderer.h"

RenderOutput::RenderOutput(Device& device, Output* output):
    output(output),
    region(output->region()),
    renderer(device, this->region, output->color_attachment_descr()) {

    uint32_t images = this->output->num_swap_images();
    this->framebuffers.reserve(images);
    for (uint32_t i = 0; i < images; ++i) {
        auto swap_image = this->output->swap_image(i);

        auto create_info = vk::FramebufferCreateInfo(
            {},
            this->renderer.final_render_pass(),
            1,
            &swap_image.view,
            this->region.extent.width,
            this->region.extent.height,
            1
        );

        this->framebuffers.push_back(device.logical->createFramebufferUnique(create_info));
    }
}

void RenderOutput::render() {
    this->output->present([this](size_t i, const auto& swap_image) {
        this->renderer.present(swap_image.command_buffer, this->framebuffers[i].get());
    });
}

DeviceRenderer::DeviceRenderer(Display* display, size_t gpu, size_t outputs):
    device(display->device(gpu)) {

    this->outputs.reserve(outputs);
    for (size_t i = 0; i < outputs; ++i) {
        this->outputs.emplace_back(this->device, display->output(gpu, i));
    }
}

void DeviceRenderer::render() {
    for (auto& output : this->outputs) {
        output.render();
    }
}

void DeviceRenderer::recreate(size_t output) {
    this->outputs[output] = RenderOutput(this->device, this->outputs[output].output);
}

DeviceRenderer::~DeviceRenderer() {
    this->device.logical->waitIdle();
}