#include "graphics/utility.h"


vk::WriteDescriptorSet write_set(vk::DescriptorSet set, const vk::DescriptorSetLayoutBinding& binding, const vk::DescriptorImageInfo& image_info) {
    return vk::WriteDescriptorSet(
        set,
        binding.binding,
        0,
        1,
        binding.descriptorType,
        &image_info,
        nullptr,
        nullptr
    );
}

vk::WriteDescriptorSet write_set(vk::DescriptorSet set, const vk::DescriptorSetLayoutBinding& binding, const vk::DescriptorBufferInfo& buffer_info) {
    return vk::WriteDescriptorSet(
        set,
        binding.binding,
        0,
        1,
        binding.descriptorType,
        nullptr,
        &buffer_info,
        nullptr
    );
}

void image_transition(vk::CommandBuffer cmd_buf, vk::Image image, ImageState src, ImageState dst) {
    auto barrier = vk::ImageMemoryBarrier(
        vk::AccessFlags(),
        vk::AccessFlags(),
        src.layout,
        dst.layout,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        image,
        vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
    );

    cmd_buf.pipelineBarrier(
        src.stage,
        dst.stage,
        vk::DependencyFlags(),
        nullptr,
        nullptr,
        barrier
    );
}
