#include "render/SimpleShaderRenderer.h"
#include <array>
#include "utility/enclosing_rect.h"
#include "resources.h"

namespace {
    vk::UniqueShaderModule create_shader(const Device& device, const std::string_view& code) {
        return device->createShaderModuleUnique(vk::ShaderModuleCreateInfo(
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

    vk::UniqueDescriptorSetLayout create_descriptor_set(const Device& device) {
        auto output_region_layout_binding = vk::DescriptorSetLayoutBinding(
            0,
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eFragment
        );

        auto output_region_layout_info = vk::DescriptorSetLayoutCreateInfo(
            {},
            1,
            &output_region_layout_binding
        );

        return device->createDescriptorSetLayoutUnique(output_region_layout_info);
    }
}

SimpleShaderRenderer::SimpleShaderRenderer(Display* display):
    display(display) {

    this->recalculate_enclosing_rect();
    this->create_resources();
}

void SimpleShaderRenderer::render() {
    const auto begin_info = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
    const auto clear_color = vk::ClearValue(vk::ClearColorValue(std::array{0.f, 0.0f, 0.f, 1.f}));

    for (auto& drsc : this->device_resources) {
        for (auto& orsc : drsc.output_resources) {
            orsc.frame_resources.submit([&](vk::CommandBuffer cmd, vk::Framebuffer framebuffer) {
                cmd.begin(&begin_info);

                auto render_pass_begin_info = vk::RenderPassBeginInfo(
                    orsc.render_pass.get(),
                    framebuffer,
                    {{0, 0}, orsc.extent},
                    1,
                    &clear_color
                );

                cmd.beginRenderPass(&render_pass_begin_info, vk::SubpassContents::eInline);
                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, orsc.pipeline.get());
                cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, drsc.pipeline_layout.get(), 0, orsc.output_region_set, nullptr);
                cmd.draw(4, 1, 0, 0);
                cmd.endRenderPass();
                cmd.end();
            });
        }
    }

    this->display->swap_buffers();
}

void SimpleShaderRenderer::recreate(size_t device, size_t output) {
    this->device_resources.clear();
    this->recalculate_enclosing_rect();
    this->create_resources();
}

void SimpleShaderRenderer::recalculate_enclosing_rect() {
    bool first = true;
    const size_t n = this->display->num_render_devices();
    for (size_t i = 0; i < n; ++i) {
        const auto& rendev = this->display->render_device(i);
        for (size_t j = 0; j < rendev.outputs; ++j) {
            Output* output = this->display->output(i, j);
            if (first) {
                this->enclosing = output->region();
                first = false;
            } else {
                this->enclosing = enclosing_rect(this->enclosing, output->region());
            }
        }
    }
}

void SimpleShaderRenderer::create_resources() {
    size_t n = this->display->num_render_devices();
    this->device_resources.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        const auto& rendev = this->display->render_device(i);
        const auto& device = rendev.device;
        uint32_t outputs = static_cast<uint32_t>(rendev.outputs);

        auto output_region_layout = create_descriptor_set(device);

        const auto descr_pool_size = vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, outputs);
        const auto descr_pool_create_info = vk::DescriptorPoolCreateInfo(
            {},
            outputs,
            1,
            &descr_pool_size
        );

        auto descr_pool = device->createDescriptorPoolUnique(descr_pool_create_info);

        const auto output_region_layouts = std::vector<vk::DescriptorSetLayout>(outputs, output_region_layout.get());

        const auto descr_alloc_info = vk::DescriptorSetAllocateInfo(
            descr_pool.get(),
            outputs,
            output_region_layouts.data()
        );

        auto output_region_sets = device->allocateDescriptorSets(descr_alloc_info);

        auto pipeline_layout_info = vk::PipelineLayoutCreateInfo(
            {},
            1,
            &output_region_layout.get()
        );

        auto pipeline_layout = device->createPipelineLayoutUnique(pipeline_layout_info);

        const auto vertex_shader = create_shader(device, resources::open("resources/test.vert"));
        const auto fragment_shader = create_shader(device, resources::open("resources/test.frag"));

        const auto shader_stages_infos = std::array{
            create_shader_info(vertex_shader.get(), vk::ShaderStageFlagBits::eVertex),
            create_shader_info(fragment_shader.get(), vk::ShaderStageFlagBits::eFragment)
        };

        const auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo();
        const auto assembly_info = vk::PipelineInputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleStrip);

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

        const auto attachment_ref = vk::AttachmentReference(
            0, // The layout(location = x) of the fragment shader
            vk::ImageLayout::eColorAttachmentOptimal
        );

        const auto subpass = vk::SubpassDescription(
            {},
            vk::PipelineBindPoint::eGraphics,
            0,
            nullptr,
            1,
            &attachment_ref
        );

        const auto dependency = vk::SubpassDependency(
            VK_SUBPASS_EXTERNAL,
            0,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::AccessFlags(),
            vk::AccessFlagBits::eColorAttachmentRead
        );

        auto output_resources = std::vector<OutputResources>();
        output_resources.reserve(outputs);
        for (size_t j = 0; j < outputs; ++j) {
            Output* output = this->display->output(i, j);
            vk::Rect2D region = output->region();

            const auto viewport = vk::Viewport(
                0.0,
                0.0,
                static_cast<float>(region.extent.width),
                static_cast<float>(region.extent.height),
                0,
                1
            );

            const auto scissor = vk::Rect2D{{0, 0}, region.extent};
            const auto viewport_info = vk::PipelineViewportStateCreateInfo({}, 1, &viewport, 1, &scissor);

            auto output_region_buffer = Buffer<OutputRegionUbo>(
                device,
                1,
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
            );

            OutputRegionUbo* ubo = output_region_buffer.map(0, 1);

            auto offset = Vec2F(static_cast<float>(region.offset.x), static_cast<float>(region.offset.y));
            auto extent = Vec2F(static_cast<float>(region.extent.width), static_cast<float>(region.extent.height));

            ubo->min = offset;
            ubo->max = offset + extent;
            ubo->offset = Vec2F(static_cast<float>(this->enclosing.offset.x), static_cast<float>(this->enclosing.offset.y));
            ubo->extent = Vec2F(static_cast<float>(this->enclosing.extent.width), static_cast<float>(this->enclosing.extent.height));

            output_region_buffer.unmap();

            const auto buffer_info = vk::DescriptorBufferInfo(
                output_region_buffer.get(),
                0,
                sizeof(OutputRegionUbo)
            );

            const auto descriptor_write = vk::WriteDescriptorSet(
                output_region_sets[j],
                0,
                0,
                1,
                vk::DescriptorType::eUniformBuffer,
                nullptr,
                &buffer_info,
                nullptr
            );

            device->updateDescriptorSets(descriptor_write, nullptr);

            const auto output_attachment = output->color_attachment_descr();

            const auto render_pass_info = vk::RenderPassCreateInfo(
                {},
                1,
                &output_attachment,
                1,
                &subpass,
                1,
                &dependency
            );

            auto render_pass =  device->createRenderPassUnique(render_pass_info);

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
                pipeline_layout.get(),
                render_pass.get()
            );

            auto pipeline = device->createGraphicsPipelineUnique(vk::PipelineCache(), pipeline_info);

            auto frame_resources = FrameResources(rendev, output, render_pass.get());

            output_resources.emplace_back(OutputResources{
                output,
                region.extent,
                std::move(output_region_buffer),
                output_region_sets[j],
                std::move(render_pass),
                std::move(pipeline),
                std::move(frame_resources)
            });
        }

        this->device_resources.emplace_back(DeviceResources{
            &rendev,
            std::move(output_region_layout),
            std::move(descr_pool),
            std::move(pipeline_layout),
            std::move(output_resources)
        });
    }
}
