#ifndef _XENODON_RENDER_RENDERCONTEXT_H
#define _XENODON_RENDER_RENDERCONTEXT_H

#include <memory>
#include <vector>
#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "backend/Display.h"
#include "backend/Output.h"
#include "render/RenderAlgorithm.h"
#include "render/RenderStats.h"
#include "camera/Camera.h"
#include "math/Vec.h"

struct RenderContext {
    struct ShaderParameters {
        Vec4F voxel_ratio;
        Vec4<unsigned> model_dim;
        float emission_coeff;
    };

    Display* display;
    std::unique_ptr<RenderAlgorithm> algorithm;
    ShaderParameters shader_params;
    vk::Rect2D display_region;
    std::vector<vk::DescriptorSetLayoutBinding> bindings;

    RenderContext(Display* display, std::unique_ptr<RenderAlgorithm>&& algorithm, const ShaderParameters& shader_params);
    void calculate_display_rect();
};

#endif
