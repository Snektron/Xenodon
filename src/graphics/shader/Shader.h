#ifndef _XENODON_GRAPHICS_SHADER_SHADER_H
#define _XENODON_GRAPHICS_SHADER_SHADER_H

#include <string_view>
#include <vulkan/vulkan.hpp>
#include "graphics/core/Device.h"

class Shader {
    vk::ShaderStageFlagBits stage;
    vk::UniqueShaderModule shader;

public:
    Shader(const Device& device, vk::ShaderStageFlagBits stage, std::string_view source);
    vk::PipelineShaderStageCreateInfo info() const;

    vk::ShaderModule get() const {
        return this->shader.get();
    }
};

#endif
