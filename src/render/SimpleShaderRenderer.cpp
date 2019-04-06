#include "render/SimpleShaderRenderer.h"
#include <array>
#include "graphics/shader/Shader.h"
#include "utility/enclosing_rect.h"
#include "resources.h"
#include "core/Logger.h"

namespace {
    namespace render_pass {
        const auto ATTACHMENT_REF = vk::AttachmentReference(
            0, // The layout(location = x) of the fragment shader output
            vk::ImageLayout::eColorAttachmentOptimal
        );

        const auto SUBPASS = vk::SubpassDescription(
            {},
            vk::PipelineBindPoint::eGraphics,
            0,
            nullptr,
            1,
            &ATTACHMENT_REF
        );

        const auto DEPENDENCY = vk::SubpassDependency(
            VK_SUBPASS_EXTERNAL,
            0,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::AccessFlags(),
            vk::AccessFlagBits::eColorAttachmentRead
        );

        vk::UniqueRenderPass create(const Device& device, vk::AttachmentDescription output_attachment) {
            return device->createRenderPassUnique({
                {},
                1,
                &output_attachment,
                1,
                &SUBPASS,
                1,
                &DEPENDENCY
            });
        }
    }

    namespace pipeline {
        const auto VERTEX_INPUT_INFO = vk::PipelineVertexInputStateCreateInfo();
        const auto ASSEMBLY_INFO = vk::PipelineInputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleStrip);
        const auto MULTISAMPLE_INFO = vk::PipelineMultisampleStateCreateInfo();

        vk::UniquePipeline create(
            const Device& device,
            Span<const vk::PipelineShaderStageCreateInfo> shaders,
            vk::PipelineLayout pipeline_layout,
            vk::RenderPass render_pass,
            vk::Extent2D extent
        ) {
            const auto viewport = vk::Viewport(
                0.0,
                0.0,
                static_cast<float>(extent.width),
                static_cast<float>(extent.height),
                0,
                1
            );

            const auto scissor = vk::Rect2D{{0, 0}, extent};
            const auto viewport_info = vk::PipelineViewportStateCreateInfo({}, 1, &viewport, 1, &scissor);

            auto rasterizer_info = vk::PipelineRasterizationStateCreateInfo();
            rasterizer_info.cullMode = vk::CullModeFlagBits::eBack;
            rasterizer_info.lineWidth = 1.0f;

            auto color_blend_attachment_info = vk::PipelineColorBlendAttachmentState();
            color_blend_attachment_info.colorWriteMask =
                  vk::ColorComponentFlagBits::eR
                | vk::ColorComponentFlagBits::eG
                | vk::ColorComponentFlagBits::eB
                | vk::ColorComponentFlagBits::eA;

            auto color_blend_info = vk::PipelineColorBlendStateCreateInfo();
            color_blend_info.attachmentCount = 1;
            color_blend_info.pAttachments = &color_blend_attachment_info;

            return device->createGraphicsPipelineUnique(vk::PipelineCache(), {
                {},
                static_cast<uint32_t>(shaders.size()),
                shaders.data(),
                &VERTEX_INPUT_INFO,
                &ASSEMBLY_INFO,
                nullptr,
                &viewport_info,
                &rasterizer_info,
                &MULTISAMPLE_INFO,
                nullptr,
                &color_blend_info,
                nullptr,
                pipeline_layout,
                render_pass
            });
        }
    }

    vk::UniqueDescriptorSetLayout create_descriptor_set_layout(const Device& device) {
        const auto output_region_layout_binding = vk::DescriptorSetLayoutBinding(
            0,
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eFragment
        );

        const auto output_region_layout_info = vk::DescriptorSetLayoutCreateInfo(
            {},
            1,
            &output_region_layout_binding
        );

        return device->createDescriptorSetLayoutUnique(output_region_layout_info);
    }

    template <typename T>
    void update_descriptor_write(const Device& device, const Buffer<T>& buffer, vk::DescriptorSet set, size_t index) {
        const auto buffer_info = vk::DescriptorBufferInfo(
            buffer.get(),
            index * sizeof(T),
            sizeof(T)
        );

        const auto descriptor_write = vk::WriteDescriptorSet(
            set,
            0,
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &buffer_info,
            nullptr
        );

        device->updateDescriptorSets(descriptor_write, nullptr);
    }
}

SimpleShaderRenderer::SimpleShaderRenderer(Display* display):
    display(display),
    start(std::chrono::system_clock::now()) {

    this->calculate_enclosing_rect();
    this->create_resources();
}

void SimpleShaderRenderer::render() {
    const auto begin_info = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
    const auto clear_color = vk::ClearValue(vk::ClearColorValue(std::array{0.f, 0.0f, 0.f, 1.f}));

    const auto now = std::chrono::system_clock::now();
    const float time = std::chrono::duration<float>(now - this->start).count();

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
                cmd.pushConstants(drsc.pipeline_layout.get(), vk::ShaderStageFlagBits::eFragment, 0, sizeof(float), static_cast<const void*>(&time));
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
    this->calculate_enclosing_rect();
    this->create_resources();
}

void SimpleShaderRenderer::calculate_enclosing_rect() {
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

        const auto descr_pool_size = vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, outputs);
        const auto descr_pool_create_info = vk::DescriptorPoolCreateInfo(
            {},
            outputs,
            1,
            &descr_pool_size
        );

        auto descr_pool = device->createDescriptorPoolUnique(descr_pool_create_info);

        auto output_region_layout = create_descriptor_set_layout(device);
        const auto output_region_layouts = std::vector<vk::DescriptorSetLayout>(outputs, output_region_layout.get());

        const auto descr_alloc_info = vk::DescriptorSetAllocateInfo(
            descr_pool.get(),
            outputs,
            output_region_layouts.data()
        );

        auto output_region_sets = device->allocateDescriptorSets(descr_alloc_info);

        const auto push_constant_range = vk::PushConstantRange(
            vk::ShaderStageFlagBits::eFragment,
            0,
            sizeof(float)
        );

        auto pipeline_layout = device->createPipelineLayoutUnique({
            {},
            1,
            &output_region_layout.get(),
            1,
            &push_constant_range
        });

        const auto vertex_shader = Shader(device, vk::ShaderStageFlagBits::eVertex, resources::open("resources/fullscreen_quad.vert"));
        const auto fragment_shader = Shader(device, vk::ShaderStageFlagBits::eFragment, resources::open("resources/test.frag"));

        const auto shaders = std::array{
            vertex_shader.info(),
            fragment_shader.info()
        };

        auto output_region_buffer = Buffer<OutputRegionUbo>(
            device,
            outputs,
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        OutputRegionUbo* ubo = output_region_buffer.map(0, outputs);

        auto output_resources = std::vector<OutputResources>();
        output_resources.reserve(outputs);
        for (size_t j = 0; j < outputs; ++j) {
            Output* output = this->display->output(i, j);
            vk::Rect2D region = output->region();

            auto offset = Vec2F(static_cast<float>(region.offset.x), static_cast<float>(region.offset.y));
            auto extent = Vec2F(static_cast<float>(region.extent.width), static_cast<float>(region.extent.height));

            ubo[j].min = offset;
            ubo[j].max = offset + extent;
            ubo[j].offset = Vec2F(static_cast<float>(this->enclosing.offset.x), static_cast<float>(this->enclosing.offset.y));
            ubo[j].extent = Vec2F(static_cast<float>(this->enclosing.extent.width), static_cast<float>(this->enclosing.extent.height));

            update_descriptor_write(device, output_region_buffer, output_region_sets[j], j);
            auto render_pass = render_pass::create(device, output->color_attachment_descr());
            auto pipeline = pipeline::create(device, shaders, pipeline_layout.get(), render_pass.get(), region.extent);
            auto frame_resources = FrameResources(rendev, output, render_pass.get());

            output_resources.emplace_back(OutputResources{
                output,
                region.extent,
                output_region_sets[j],
                std::move(render_pass),
                std::move(pipeline),
                std::move(frame_resources)
            });
        }

        output_region_buffer.unmap();

        this->device_resources.emplace_back(DeviceResources{
            &rendev,
            std::move(output_region_layout),
            std::move(descr_pool),
            std::move(pipeline_layout),
            std::move(output_region_buffer),
            std::move(output_resources)
        });
    }
}
