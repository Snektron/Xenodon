#include "render/Renderer.h"
#include <array>
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

Renderer::Renderer(DeviceContext& device_context, vk::Rect2D area, vk::AttachmentDescription& target):
    device_context(device_context),
    area(area) {
    this->init_render_pass(target);
}

void Renderer::render(vk::CommandBuffer buf, vk::Framebuffer target) {
    const auto begin_info = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
    const auto clear_color = vk::ClearValue(vk::ClearColorValue(std::array<float, 4>{0.f, 0.0f, 0.f, 1.f}));

    buf.begin(&begin_info);

    auto render_pass_begin_info = vk::RenderPassBeginInfo(
        this->render_pass.get(),
        target,
        this->area,
        1,
        &clear_color
    );

    buf.beginRenderPass(&render_pass_begin_info, vk::SubpassContents::eInline);
    buf.bindPipeline(vk::PipelineBindPoint::eGraphics, this->pipeline.get());
    buf.draw(4, 1, 0, 0);
    buf.endRenderPass();
    buf.end();
}

vk::RenderPass Renderer::final_render_pass() const {
    return this->render_pass.get();
}

void Renderer::init_render_pass(vk::AttachmentDescription& target) {
    auto& device = this->device_context.device.get();
    const auto vertex_shader = create_shader(device, resources::open("resources/test.vert"));
    const auto fragment_shader = create_shader(device, resources::open("resources/test.frag"));

    const auto shader_stages_infos = std::array{
        create_shader_info(vertex_shader.get(), vk::ShaderStageFlagBits::eVertex),
        create_shader_info(fragment_shader.get(), vk::ShaderStageFlagBits::eFragment)
    };

    const auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo();
    const auto assembly_info = vk::PipelineInputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleStrip);

    const auto viewport = vk::Viewport(
        static_cast<float>(area.offset.x),
        static_cast<float>(area.offset.y),
        static_cast<float>(area.extent.width),
        static_cast<float>(area.extent.height),
        0,
        1
    );

    const auto& scissor = this->area;
    const auto viewport_info = vk::PipelineViewportStateCreateInfo({}, 1, &viewport, 1, &scissor);

    auto rasterizer_info = vk::PipelineRasterizationStateCreateInfo();
    rasterizer_info.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer_info.lineWidth = 1.0f;

    const auto multisample_info = vk::PipelineMultisampleStateCreateInfo();

    auto color_blend_attachment_info = vk::PipelineColorBlendAttachmentState();
    color_blend_attachment_info.colorWriteMask =
          vk::ColorComponentFlagBits::eR
        | vk::ColorComponentFlagBits::eG
        | vk::ColorComponentFlagBits::eB
        | vk::ColorComponentFlagBits::eA;

    auto color_blend_info = vk::PipelineColorBlendStateCreateInfo();
    color_blend_info.attachmentCount = 1;
    color_blend_info.pAttachments = &color_blend_attachment_info;

    auto pipeline_layout_info = vk::PipelineLayoutCreateInfo();
    this->layout = device.createPipelineLayoutUnique(pipeline_layout_info);

    auto attachment_ref = vk::AttachmentReference(
        0, // The layout(location = x) of the fragment shader
        vk::ImageLayout::eColorAttachmentOptimal
    );

    auto subpass = vk::SubpassDescription(
        {},
        vk::PipelineBindPoint::eGraphics,
        0,
        nullptr,
        1,
        &attachment_ref
    );

    auto dependency = vk::SubpassDependency(
        VK_SUBPASS_EXTERNAL,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::AccessFlags(),
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentRead
    );

    auto render_pass_info = vk::RenderPassCreateInfo(
        {},
        1,
        &target,
        1,
        &subpass,
        1,
        &dependency
    );

    this->render_pass = device.createRenderPassUnique(render_pass_info);

    auto pipeline_info = vk::GraphicsPipelineCreateInfo(
        {},
        shader_stages_infos.size(),
        shader_stages_infos.data(),
        &vertex_input_info,
        &assembly_info,
        nullptr,
        &viewport_info,
        &rasterizer_info,
        &multisample_info,
        nullptr,
        &color_blend_info,
        nullptr,
        this->layout.get(),
        this->render_pass.get()
    );

    this->pipeline = device.createGraphicsPipelineUnique(vk::PipelineCache(), pipeline_info);
}