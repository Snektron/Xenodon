#include "render/DeviceRenderer.h"
#include "utility/bind_member.h"

RenderOutput::RenderOutput(const RenderDevice& rendev, Output* output):
    output(output),
    renderer(std::make_unique<TestRenderer>(rendev.device, output->region(), output->color_attachment_descr())),
    resources(rendev, output, this->renderer->final_render_pass()) {
}

void RenderOutput::render() {
    this->resources.submit(bind_member(this->renderer.get(), &TestRenderer::present));
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