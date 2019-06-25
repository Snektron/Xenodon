#include "render/MultiplexRenderer.h"

MultiplexRenderer::MultiplexRenderer(Display* display, std::unique_ptr<RenderAlgorithm>&& algorithm, const ShaderParameters& shader_params):
    ctx(std::make_shared<RenderContext>(display, std::move(algorithm), shader_params)) {

    const size_t n = display->num_render_devices();
    for (size_t i = 0; i < n; ++i) {
        this->renderers.emplace_back(this->ctx, i);
    }
}

void MultiplexRenderer::recreate(size_t device, size_t output) {
    this->ctx->calculate_display_rect();
    this->renderers[device].recreate(output);

    for (auto& renderer : this->renderers) {
        renderer.resize();
    }
}

void MultiplexRenderer::render(const Camera& cam) {
    for (auto& renderer : this->renderers) {
        renderer.render(cam);
    }

    this->ctx->display->swap_buffers();

    for (auto& renderer : this->renderers) {
        renderer.collect_stats();
    }
}

RenderStats MultiplexRenderer::stats() const {
    auto stats = RenderStats();

    for (const auto& renderer : this->renderers) {
        stats.combine(renderer.stats());
    }

    return stats;
}