#include "render/RenderContext.h"
#include <utility>
#include <algorithm>
#include "core/Logger.h"
#include "utility/rect_union.h"

namespace {
    // Standard bindings all RenderAlgorithm shaders should have
    const auto COMMON_BINDINGS = std::array {
        vk::DescriptorSetLayoutBinding(
            0, // layout(binding = 0) uniform OutputRegionBuffer
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eCompute
        ),
        vk::DescriptorSetLayoutBinding(
            1, // layout(binding = 1) restrict writeonly uniform image2D render_target
            vk::DescriptorType::eStorageImage,
            1,
            vk::ShaderStageFlagBits::eCompute
        )
    };
}

RenderContext::RenderContext(Display* display, std::unique_ptr<RenderAlgorithm>&& algorithm, const ShaderParameters& shader_params):
    display(display),
    algorithm(std::move(algorithm)),
    shader_params(shader_params) {
    this->calculate_display_rect();

    std::copy(COMMON_BINDINGS.begin(), COMMON_BINDINGS.end(), std::back_inserter(this->bindings));

    for (auto [binding, type] : this->algorithm->bindings()) {
        this->bindings.emplace_back(
            binding,
            type,
            1,
            vk::ShaderStageFlagBits::eCompute
        );
    }
}

void RenderContext::calculate_display_rect() {
    bool first = true;
    const size_t n = this->display->num_render_devices();
    for (size_t i = 0; i < n; ++i) {
        const auto& rendev = this->display->render_device(i);
        for (size_t j = 0; j < rendev.outputs; ++j) {
            Output* output = this->display->output(i, j);
            if (first) {
                this->display_region = output->region();
                first = false;
            } else {
                this->display_region = rect_union(this->display_region, output->region());
            }
        }
    }

    LOGGER.log("Total resolution: {}x{} pixels", this->display_region.extent.width, this->display_region.extent.height);
}
