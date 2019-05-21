#ifndef _XENODON_RENDER_RENDERER_H
#define _XENODON_RENDER_RENDERER_H

#include <vector>
#include <chrono>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "backend/Display.h"
#include "backend/Output.h"
#include "backend/RenderDevice.h"
#include "graphics/memory/Buffer.h"
#include "render/RenderAlgorithm.h"
#include "camera/Camera.h"
#include "math/Vec.h"

class Renderer {
public:
    struct ShaderParameters {
        float density;
    };

private:
    struct UniformBuffer {
        vk::Rect2D output_region;
        vk::Rect2D display_region;
        ShaderParameters params;
    };

    struct PushConstantBuffer {
        // the Camera struct cant be used here directly because of
        // different alignment requirements of CPU and GPU.
        struct {
            Vec4F dir, pos, up;
        } camera;

        float time;
    };

    static_assert(sizeof(PushConstantBuffer) <= 128, "Vulkan minimum supported push constant range is maximum 128 bytes");

    struct OutputResources {
        Output* output;

        vk::Rect2D region;
        vk::UniquePipeline pipeline;

        Span<vk::DescriptorSet> descriptor_sets;
        std::vector<vk::UniqueCommandBuffer> command_buffers;
    };

    struct DeviceResources {
        const RenderDevice* rendev;
        std::unique_ptr<RenderResources> resources;

        vk::UniqueDescriptorSetLayout descriptor_set_layout;
        vk::UniqueDescriptorPool descriptor_pool;
        std::vector<vk::DescriptorSet> descriptor_sets;

        vk::UniquePipelineLayout pipeline_layout;

        Buffer<UniformBuffer> uniform_buffer;

        std::vector<OutputResources> output_resources;
    };

    Display* display;
    const RenderAlgorithm* algorithm;
    ShaderParameters shader_params;
    vk::Rect2D display_region;
    std::vector<DeviceResources> device_resources;
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    std::chrono::system_clock::time_point start;

public:
    Renderer(Display* display, const RenderAlgorithm* algorithm, const ShaderParameters& shader_params);
    void recreate(size_t device, size_t output);
    void render(const Camera& cam);

private:
    void create_resources();
    void create_descriptor_sets();
    void create_command_buffers();
    void update_descriptor_sets();
    void upload_uniform_buffers();
    vk::UniqueDescriptorPool create_descriptor_pool(const Device& device, uint32_t sets);
    void calculate_display_rect();
};

#endif
