#ifndef _XENODON_RENDER_SVORAYTRACEALGORITHM_H
#define _XENODON_RENDER_SVORAYTRACEALGORITHM_H

#include <string_view>
#include <memory>
#include <cstddef>
#include "render/RenderAlgorithm.h"
#include "model/Octree.h"
#include "backend/RenderDevice.h"
#include "graphics/memory/Buffer.h"

class SvoRaytraceResources: public RenderResources {
    Buffer<Octree::Node> node_buffer;
    size_t size;

public:
    SvoRaytraceResources(const RenderDevice& rendev, const Octree& octree);
    void update_descriptors(vk::DescriptorSet set) const override;
};

class SvoRaytraceAlgorithm: public RenderAlgorithm {
    std::string_view shader_source;
    std::shared_ptr<Octree> octree;

public:
    SvoRaytraceAlgorithm(std::string_view shader_source, std::shared_ptr<Octree> octree);
    std::string_view shader() const override;
    Span<Binding> bindings() const override;
    std::unique_ptr<RenderResources> upload_resources(const RenderDevice& rendev) const override;
};

#endif
