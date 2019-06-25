#ifndef _XENODON_RENDER_RENDERER_H
#define _XENODON_RENDER_RENDERER_H

#include <vector>
#include <memory>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "backend/Display.h"
#include "backend/Output.h"
#include "backend/RenderDevice.h"
#include "graphics/memory/Buffer.h"
#include "render/RenderAlgorithm.h"
#include "render/RenderStats.h"
#include "render/RenderContext.h"
#include "camera/Camera.h"
#include "math/Vec.h"

class Renderer {
    using ShaderParameters = RenderContext::ShaderParameters;

    struct UniformBuffer {
        vk::Rect2D output_region;
        vk::Rect2D display_region;
        ShaderParameters params;
    };

    struct PushConstantBuffer {
        // the Camera struct cant be used here directly because of
        // different alignment requirements of CPU and GPU.
        struct {
            Vec4F forward;
            Vec4F up;
            Vec4F translation_scaled;
        } camera;
    };

    static_assert(sizeof(PushConstantBuffer) <= 128, "Vulkan minimum supported push constant range is maximum 128 bytes");

    struct OutputResources {
        Output* output;

        vk::Rect2D region;

        Span<vk::DescriptorSet> descriptor_sets;
        std::vector<vk::UniqueCommandBuffer> command_buffers;
    };

    std::shared_ptr<RenderContext> ctx;
    size_t device_index;

    const RenderDevice* rendev;
    std::unique_ptr<RenderResources> resources;
    RenderStatsCollector stats_collector;

    vk::UniqueDescriptorSetLayout descriptor_set_layout;
    vk::UniqueDescriptorPool descriptor_pool;
    std::vector<vk::DescriptorSet> descriptor_sets;

    vk::UniquePipelineLayout pipeline_layout;
    vk::UniquePipeline pipeline;

    std::unique_ptr<Buffer<UniformBuffer>> uniform_buffer;

    std::vector<OutputResources> output_resources;

public:
    Renderer(std::shared_ptr<RenderContext> ctx, size_t device_index);
    void recreate(size_t output);
    void resize();
    void render(const Camera& cam);
    void collect_stats();
    RenderStats stats() const;

private:
    void create_resources();
    void create_pipeline();
    void create_descriptor_sets();
    void create_command_buffers();
    void update_descriptor_sets();
    void upload_uniform_buffers();
    vk::UniqueDescriptorPool create_descriptor_pool(const Device& device, uint32_t sets);
};

#endif
