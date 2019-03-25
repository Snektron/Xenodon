#include "render/DeviceRenderer.h"

RenderOutput::RenderOutput(Device& device, Screen* screen):
    screen(screen),
    region(screen->region()),
    renderer(device, this->region.extent, screen->color_attachment_descr()) {

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
    size_t idx = this->screen->active_index();
    auto swap_image = this->screen->active_image();

    this->renderer.present(swap_image.command_buffer, this->framebuffers[idx].get());
    this->screen->swap_buffers();
}

DeviceRenderer::DeviceRenderer(Display* display, size_t gpu, size_t screens):
    device(display->device_at(gpu)) {

    this->outputs.reserve(screens);
    for (size_t i = 0; i < screens; ++i) {
        this->outputs.emplace_back(this->device, display->screen_at(gpu, i));
    }
}

void DeviceRenderer::render() {
    for (auto& output : this->outputs) {
        output.render();
    }
}

void DeviceRenderer::recreate(size_t screen) {
    auto it = this->outputs.begin() + static_cast<std::vector<RenderOutput>::difference_type>(screen);
    this->outputs.emplace(it, this->device, this->outputs[screen].screen);
}

DeviceRenderer::~DeviceRenderer() {
    this->device.logical->waitIdle();
}