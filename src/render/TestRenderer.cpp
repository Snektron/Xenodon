#include "render/TestRenderer.h"
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

TestRenderer::TestRenderer(const Device2& device, vk::Rect2D region, vk::AttachmentDescription output_attachment):
    device(device),
    region(region),
    output_region(device, sizeof(OutputRegion), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) {

    auto* output_region = reinterpret_cast<OutputRegion*>(this->device->mapMemory(this->output_region.memory(), 0, sizeof(OutputRegion)));

    this->device->unmapMemory(this->output_region.memory());

    const auto vertex_shader = create_shader(this->device.get(), resources::open("resources/test.vert"));
    const auto fragment_shader = create_shader(this->device.get(), resources::open("resources/test.frag"));

    auto render_region_binding = vk::DescriptorSetLayoutBinding(
        0,
        vk::DescriptorType::eUniformBuffer,
        1,
        vk::ShaderStageFlagBits::eFragment
    );

    auto layout_create_info = vk::DescriptorSetLayoutCreateInfo(
        {},
        1,
        &render_region_binding
    );

    this->descriptor_layout = device->createDescriptorSetLayoutUnique(layout_create_info);

    const auto shader_stages_infos = std::array{
        create_shader_info(vertex_shader.get(), vk::ShaderStageFlagBits::eVertex),
        create_shader_info(fragment_shader.get(), vk::ShaderStageFlagBits::eFragment)
    };

    const auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo();
    const auto assembly_info = vk::PipelineInputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleStrip);

    const auto viewport = vk::Viewport(
        0.0,
        0.0,
        static_cast<float>(this->region.extent.width),
        static_cast<float>(this->region.extent.height),
        0,
        1
    );

    const auto scissor = vk::Rect2D{{0, 0}, this->region.extent};
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

    auto pipeline_layout_info = vk::PipelineLayoutCreateInfo(
        {},
        1,
        &this->descriptor_layout.get()
    );

    this->layout = this->device->createPipelineLayoutUnique(pipeline_layout_info);

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
        &output_attachment,
        1,
        &subpass,
        1,
        &dependency
    );

    this->render_pass = this->device->createRenderPassUnique(render_pass_info);

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

    this->pipeline = this->device->createGraphicsPipelineUnique(vk::PipelineCache(), pipeline_info);
}

void TestRenderer::present(vk::CommandBuffer buf, vk::Framebuffer target) {
    const auto begin_info = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
    const auto clear_color = vk::ClearValue(vk::ClearColorValue(std::array<float, 4>{0.f, 0.0f, 0.f, 1.f}));

    buf.begin(&begin_info);

    auto render_pass_begin_info = vk::RenderPassBeginInfo(
        this->render_pass.get(),
        target,
        {{0, 0}, this->region.extent},
        1,
        &clear_color
    );

    buf.beginRenderPass(&render_pass_begin_info, vk::SubpassContents::eInline);
    buf.bindPipeline(vk::PipelineBindPoint::eGraphics, this->pipeline.get());
    buf.draw(4, 1, 0, 0);
    buf.endRenderPass();
    buf.end();
}

vk::RenderPass TestRenderer::final_render_pass() const {
    return this->render_pass.get();
}
