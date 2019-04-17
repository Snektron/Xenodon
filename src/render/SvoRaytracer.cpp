#include "render/SvoRaytracer.h"
#include <array>
#include "graphics/shader/Shader.h"
#include "utility/enclosing_rect.h"
#include "resources.h"
#include "core/Logger.h"

SvoRaytracer::SvoRaytracer(Display* display, const Octree& model):
    display(display),
    start(std::chrono::system_clock::now()),
    model(model) {

    this->calculate_enclosing_rect();
    this->create_resources();
}

void SvoRaytracer::render() {
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
                    {{0, 0}, orsc.region.extent},
                    1,
                    &clear_color
                );

                cmd.beginRenderPass(&render_pass_begin_info, vk::SubpassContents::eInline);
                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, orsc.pipeline.get());
                cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, drsc.pipeline_layout.get(), 0, orsc.descr_set, nullptr);
                cmd.pushConstants(drsc.pipeline_layout.get(), vk::ShaderStageFlagBits::eFragment, 0, sizeof(float), static_cast<const void*>(&time));
                cmd.draw(4, 1, 0, 0);
                cmd.endRenderPass();
                cmd.end();
            });
        }
    }

    this->display->swap_buffers();
}

void SvoRaytracer::recreate(size_t device, size_t output) {
    this->device_resources.clear();
    this->calculate_enclosing_rect();
    this->create_resources();
}

void SvoRaytracer::calculate_enclosing_rect() {
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

void SvoRaytracer::create_resources() {
    size_t n = this->display->num_render_devices();
    this->device_resources.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        const auto& rendev = this->display->render_device(i);
        const auto& device = rendev.device;
        uint32_t outputs = static_cast<uint32_t>(rendev.outputs);

        const auto pool_sizes = std::array{
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, outputs),
            vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, outputs)
        };

        const auto descr_pool_create_info = vk::DescriptorPoolCreateInfo(
            {},
            outputs,
            static_cast<uint32_t>(pool_sizes.size()),
            pool_sizes.data()
        );

        auto descr_pool = device->createDescriptorPoolUnique(descr_pool_create_info);

        auto layout_bindings = std::array {
            vk::DescriptorSetLayoutBinding(
                0, // layout(binding = 0)
                vk::DescriptorType::eUniformBuffer,
                1,
                vk::ShaderStageFlagBits::eFragment
            ),
            vk::DescriptorSetLayoutBinding(
                1, // layout(binding = 1)
                vk::DescriptorType::eStorageBuffer,
                1,
                vk::ShaderStageFlagBits::eFragment
            ),
        };

        auto descr_set_layout = device->createDescriptorSetLayoutUnique({
            {},
            static_cast<uint32_t>(layout_bindings.size()),
            layout_bindings.data()
        });

        const auto descr_set_layouts = std::vector<vk::DescriptorSetLayout>(outputs, descr_set_layout.get());

        const auto descr_alloc_info = vk::DescriptorSetAllocateInfo(
            descr_pool.get(),
            outputs,
            descr_set_layouts.data()
        );

        auto descr_sets = device->allocateDescriptorSets(descr_alloc_info);

        const auto push_constant_range = vk::PushConstantRange(
            vk::ShaderStageFlagBits::eFragment,
            0,
            sizeof(float)
        );

        auto pipeline_layout = device->createPipelineLayoutUnique({
            {},
            1,
            &descr_set_layout.get(),
            1,
            &push_constant_range
        });

        const auto vertex_shader = Shader(device, vk::ShaderStageFlagBits::eVertex, resources::open("resources/fullscreen_quad.vert"));
        const auto fragment_shader = Shader(device, vk::ShaderStageFlagBits::eFragment, resources::open("resources/svo.frag"));

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

        const size_t num_nodes = this->model.data().size();

        auto tree_buffer = Buffer<Octree::Node>(
            device,
            num_nodes,
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        );

        auto output_resources = std::vector<OutputResources>();
        output_resources.reserve(outputs);
        for (size_t j = 0; j < outputs; ++j) {
            Output* output = this->display->output(i, j);
            vk::Rect2D region = output->region();

            const auto uniform_buffer_info = vk::DescriptorBufferInfo(
                output_region_buffer.get(),
                j * sizeof(OutputRegionUbo),
                sizeof(OutputRegionUbo)
            );

            const auto tree_buffer_info = vk::DescriptorBufferInfo(
                tree_buffer.get(),
                0,
                sizeof(Octree::Node) * num_nodes
            );

            const auto descriptor_writes = std::array{
                vk::WriteDescriptorSet(
                    descr_sets[j],
                    0, // binding
                    0,
                    1,
                    vk::DescriptorType::eUniformBuffer,
                    nullptr,
                    &uniform_buffer_info,
                    nullptr
                ),
                vk::WriteDescriptorSet(
                    descr_sets[j],
                    1, // binding
                    0,
                    1,
                    vk::DescriptorType::eStorageBuffer,
                    nullptr,
                    &tree_buffer_info,
                    nullptr
                )
            };

            device->updateDescriptorSets(descriptor_writes, nullptr);

            auto render_pass = device.create_present_render_pass(0, output->color_attachment_descr());
            auto pipeline = device.create_pipeline(shaders, pipeline_layout.get(), render_pass.get(), region.extent);
            auto frame_resources = FrameResources(rendev, output, render_pass.get());

            output_resources.emplace_back(OutputResources{
                output,
                region,
                descr_sets[j],
                std::move(render_pass),
                std::move(pipeline),
                std::move(frame_resources)
            });
        }

        this->device_resources.emplace_back(DeviceResources{
            &rendev,
            std::move(descr_set_layout),
            std::move(descr_pool),
            std::move(pipeline_layout),
            std::move(output_region_buffer),
            std::move(tree_buffer),
            std::move(output_resources)
        });
    }

    this->update_output_regions();
    this->update_tree_buffers();
}

void SvoRaytracer::update_output_regions() {
    for (auto& drsc : this->device_resources) {
        OutputRegionUbo* ubo = drsc.output_region_buffer.map(0, drsc.rendev->outputs);
        size_t j = 0;

        for (auto& orsc : drsc.output_resources) {
            auto offset = Vec2F(static_cast<float>(orsc.region.offset.x), static_cast<float>(orsc.region.offset.y));
            auto extent = Vec2F(static_cast<float>(orsc.region.extent.width), static_cast<float>(orsc.region.extent.height));

            ubo[j].min = offset;
            ubo[j].max = offset + extent;
            ubo[j].offset = Vec2F(static_cast<float>(this->enclosing.offset.x), static_cast<float>(this->enclosing.offset.y));
            ubo[j].extent = Vec2F(static_cast<float>(this->enclosing.extent.width), static_cast<float>(this->enclosing.extent.height));
            ++j;
        }

        drsc.output_region_buffer.unmap();
    }
}

void SvoRaytracer::update_tree_buffers() {
    const Span<const Octree::Node> nodes = this->model.data();

    const auto copy_info = vk::BufferCopy{
        0,
        0,
        nodes.size() * sizeof(Octree::Node)
    };

    for (auto& drsc : this->device_resources) {
        const auto& rendev = *drsc.rendev;
        const auto& device = rendev.device;

        // Sadly this has to be done for every GPU seperately...
        auto staging_buffer = Buffer<Octree::Node>(
            device,
            nodes.size(),
            vk::BufferUsageFlagBits::eTransferSrc,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
        );

        Octree::Node* staging_nodes = staging_buffer.map(0, nodes.size());

        for (size_t i = 0; i < nodes.size(); ++i) {
            staging_nodes[i] = nodes[i];
        }

        staging_buffer.unmap();

        rendev.graphics_command_pool.one_time_submit([&](vk::CommandBuffer cmd_buf) {
            cmd_buf.copyBuffer(staging_buffer.get(), drsc.tree_buffer.get(), copy_info);
        });
    }
}