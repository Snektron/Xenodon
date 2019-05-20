#ifndef _XENODON_RENDER_RENDERALGORITHM_H
#define _XENODON_RENDER_RENDERALGORITHM_H

#include <string_view>
#include <memory>
#include <vulkan/vulkan.hpp>
#include "backend/RenderDevice.h"
#include "utility/Span.h"

struct Binding {
    uint32_t binding;
    vk::DescriptorType type;
};

struct RenderResources {
    virtual ~RenderResources() = default;
    virtual void update_descriptors(vk::DescriptorSet set) const = 0;
};

struct RenderAlgorithm {
    virtual ~RenderAlgorithm() = default;
    virtual std::string_view shader() const = 0;
    virtual Span<Binding> bindings() const = 0;
    virtual std::unique_ptr<RenderResources> upload_resources(const RenderDevice& rendev) const = 0;
};


#endif
