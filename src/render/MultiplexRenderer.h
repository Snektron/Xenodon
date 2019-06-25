#ifndef _XENODON_RENDER_MULTIPLEXRENDERER_H
#define _XENODON_RENDER_MULTIPLEXRENDERER_H

#include <memory>
#include <vector>
#include <cstddef>
#include "render/Renderer.h"
#include "render/RenderContext.h"
#include "render/RenderStats.h"
#include "camera/Camera.h"
#include "backend/Display.h"

class MultiplexRenderer {
    std::shared_ptr<RenderContext> ctx;
    std::vector<Renderer> renderers;

public:
    using ShaderParameters = RenderContext::ShaderParameters;

    MultiplexRenderer(Display* display, std::unique_ptr<RenderAlgorithm>&& algorithm, const ShaderParameters& shader_params);
    void recreate(size_t device, size_t output);
    void render(const Camera& cam);
    RenderStats stats() const;
};

#endif
