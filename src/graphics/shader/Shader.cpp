#include "graphics/shader/Shader.h"

Shader::Shader(const Device& device, vk::ShaderStageFlagBits stage, std::string_view source):
    stage(stage) {
    this->shader = device->createShaderModuleUnique({
        {},
        source.size(),
        reinterpret_cast<const uint32_t*>(source.data())
    });
}

vk::PipelineShaderStageCreateInfo Shader::info() const {
    return vk::PipelineShaderStageCreateInfo(
        {},
        this->stage,
        this->shader.get(),
        "main"
    );
}