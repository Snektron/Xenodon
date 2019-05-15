#ifndef _XENODON_RENDER_RENDERALGORITHM_H
#define _XENODON_RENDER_RENDERALGORITHM_H

#include <string_view>
#include <memory>
#include <vulkan/vulkan.hpp>
#include "graphics/core/Device.h"
#include "utility/Span.h"

struct RenderAlgorithmInstance {
    virtual ~RenderAlgorithmInstance() = 0;
};

struct RenderAlgorithm {
    virtual ~RenderAlgorithm() = default;
    virtual std::string_view shader() const = 0;
    virtual Span<vk::DescriptorSetLayoutBinding> bindings() const = 0;
    virtual std::unique_ptr<RenderAlgorithmInstance> instantiate(const Device& device) const = 0;
};


#endif
