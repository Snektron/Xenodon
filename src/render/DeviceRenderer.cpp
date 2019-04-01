#include "render/DeviceRenderer.h"

RenderOutput::RenderOutput(Device& device, Screen* screen):
    screen(screen),
    region(screen->region()),
    renderer(device, this->region, screen->color_attachment_descr()) {

    uint32_t images = this->screen->num_swap_images();
    this->framebuffers.reserve(images);
    for (uint32_t i = 0; i < images; ++i) {
        auto swap_image = this->screen->swap_image(i);

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
    this->screen->present([this](size_t i, const auto& swap_image) {
        this->renderer.present(swap_image.command_buffer, this->framebuffers[i].get());
    });
}

DeviceRenderer::DeviceRenderer(Display* display, size_t gpu, size_t screens):
    device(display->device(gpu)) {

    this->outputs.reserve(screens);
    for (size_t i = 0; i < screens; ++i) {
        this->outputs.emplace_back(this->device, display->screen(gpu, i));
    }
}

void DeviceRenderer::render() {
    for (auto& output : this->outputs) {
        output.render();
    }
}

void DeviceRenderer::recreate(size_t screen) {
    this->outputs[screen] = RenderOutput(this->device, this->outputs[screen].screen);
}

DeviceRenderer::~DeviceRenderer() {
    this->device.logical->waitIdle();
}