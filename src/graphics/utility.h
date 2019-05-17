#ifndef _XENODON_GRAPHICS_UTILITY_H
#define _XENODON_GRAPHICS_UTILITY_H

#include <vulkan/vulkan.hpp>

struct ImageState {
    vk::ImageLayout layout;
    vk::PipelineStageFlagBits stage;
};

vk::WriteDescriptorSet write_set(vk::DescriptorSet set, const vk::DescriptorSetLayoutBinding& binding, const vk::DescriptorImageInfo& image_info);
vk::WriteDescriptorSet write_set(vk::DescriptorSet set, const vk::DescriptorSetLayoutBinding& binding, const vk::DescriptorBufferInfo& buffer_info);

void image_transition(vk::CommandBuffer cmd_buf, vk::Image image, ImageState src, ImageState dst);

#endif
