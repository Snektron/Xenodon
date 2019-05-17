#ifndef _XENODON_RENDER_RENDERALGORITHM_H
#define _XENODON_RENDER_RENDERALGORITHM_H

#include <string_view>
#include <memory>
#include <vulkan/vulkan.hpp>
#include "backend/RenderDevice.h"
#include "utility/Span.h"

struct RenderAlgorithmInstance {
    virtual ~RenderAlgorithmInstance() = 0;
    virtual void update_descriptors(vk::DescriptorSet set) = 0;
    virtual void upload_buffers() = 0;
};

struct RenderAlgorithm {
    virtual ~RenderAlgorithm() = default;
    virtual std::string_view shader() const = 0;
    virtual Span<vk::DescriptorSetLayoutBinding> bindings() const = 0;
    virtual std::unique_ptr<RenderAlgorithmInstance> instantiate(const RenderDevice& rendev) const = 0;
};


#endif
