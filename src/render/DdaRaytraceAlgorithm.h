#ifndef _XENODON_RENDER_DDARAYTRACEALGORITHM_H
#define _XENODON_RENDER_DDARAYTRACEALGORITHM_H

#include <memory>
#include "render/RenderAlgorithm.h"
#include "model/Grid.h"
#include "backend/RenderDevice.h"
#include "graphics/memory/Texture3D.h"

class DdaRaytraceResources: public RenderResources {
    Texture3D grid_texture;
    vk::UniqueSampler sampler;

public:
    DdaRaytraceResources(const RenderDevice& rendev, const Grid& grid);
    void update_descriptors(vk::DescriptorSet set) const override;
};

class DdaRaytraceAlgorithm: public RenderAlgorithm {
    std::shared_ptr<Grid> grid;

public:
    DdaRaytraceAlgorithm(std::shared_ptr<Grid> grid);
    std::string_view shader() const override;
    Span<Binding> bindings() const override;
    std::unique_ptr<RenderResources> upload_resources(const RenderDevice& rendev) const override;
};

#endif
