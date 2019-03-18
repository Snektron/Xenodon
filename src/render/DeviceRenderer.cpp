#include "render/DeviceRenderer.h"
#include "resources.h"

namespace {
    vk::UniqueShaderModule create_shader(vk::Device device, const std::string_view& code) {
        return device.createShaderModuleUnique(vk::ShaderModuleCreateInfo(
            {},
            code.size(),
            reinterpret_cast<const uint32_t*>(code.data())
        ));
    }

    vk::PipelineShaderStageCreateInfo create_shader_info(const vk::ShaderModule& shader, vk::ShaderStageFlagBits stage) {
        return vk::PipelineShaderStageCreateInfo(
            {},
            stage,
            shader,
            "main"
        );
    }
}

RenderOutput::RenderOutput(Device& device, vk::RenderPass render_pass, Screen* screen):
    screen(screen),
    region(screen->region()) {

    uint32_t images = this->screen->num_swap_images();
    this->framebuffers.reserve(images);
    for (uint32_t i = 0; i < images; ++i) {
        auto swap_image = this->screen->swap_image(i);

        auto create_info = vk::FramebufferCreateInfo(
            {},
            render_pass,
            1,
            &swap_image.view,
            this->region.extent.width,
            this->region.extent.height,
            1
        );

        this->framebuffers.push_back(device.logical->createFramebufferUnique(create_info));
    }
}

DeviceRenderer::DeviceRenderer(Display* display, size_t gpu, size_t screens):
    device(display->device_at(gpu)) {
    this->outputs.reserve(screens);

    const auto vertex_shader = create_shader(this->device.logical.get(), resources::open("resources/test.vert"));
    const auto fragment_shader = create_shader(this->device.logical.get(), resources::open("resources/test.frag"));

    const auto shader_stages_infos = std::array{
        create_shader_info(vertex_shader.get(), vk::ShaderStageFlagBits::eVertex),
        create_shader_info(fragment_shader.get(), vk::ShaderStageFlagBits::eFragment)
    };

    const auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo();
    const auto assembly_info = vk::PipelineInputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleStrip);
}

void DeviceRenderer::render() {

}

void DeviceRenderer::recreate(size_t screen) {

}