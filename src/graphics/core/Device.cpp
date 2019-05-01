#include "graphics/core/Device.h"
#include <vector>
#include <algorithm>

Device::Device(const PhysicalDevice& physdev, Span<vk::DeviceQueueCreateInfo> queue_families, Span<const char*> extensions):
    physdev(physdev.get()) {

    this->dev = this->physdev.createDeviceUnique({
        {},
        static_cast<uint32_t>(queue_families.size()),
        queue_families.data(),
        0,
        nullptr,
        static_cast<uint32_t>(extensions.size()),
        extensions.data()
    });
}

Device::Device(const PhysicalDevice& physdev, Span<uint32_t> queue_families, Span<const char*> extensions):
    physdev(physdev.get()) {
    float priority = 1.0f;
    auto queue_create_infos = std::vector<vk::DeviceQueueCreateInfo>(queue_families.size());

    std::transform(
        queue_families.begin(),
        queue_families.end(),
        queue_create_infos.begin(),
        [&priority](uint32_t family) {
            return vk::DeviceQueueCreateInfo {
                {},
                family,
                1,
                &priority
            };
        }
    );

    std::sort(
        queue_create_infos.begin(),
        queue_create_infos.end(),
        [](const auto& a, const auto& b){
            return a.queueFamilyIndex < b.queueFamilyIndex;
        }
    );

    const auto last = std::unique(queue_create_infos.begin(), queue_create_infos.end());

    this->dev = this->physdev.createDeviceUnique({
        {},
        static_cast<uint32_t>(std::distance(queue_create_infos.begin(), last)),
        queue_create_infos.data(),
        0,
        nullptr,
        static_cast<uint32_t>(extensions.size()),
        extensions.data(),
        nullptr
    });
}

std::optional<uint32_t> Device::find_memory_type(uint32_t filter, vk::MemoryPropertyFlags flags) const {
    auto mem_props = this->physdev.getMemoryProperties();
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; ++i) {
        if (filter & (1 << i) && (mem_props.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }

    return std::nullopt;
}

vk::DeviceMemory Device::allocate(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags flags) const {
    auto alloc_info = vk::MemoryAllocateInfo(
        requirements.size,
        this->find_memory_type(requirements.memoryTypeBits, flags).value()
    );

    return this->dev->allocateMemory(alloc_info);
}

vk::UniqueDeviceMemory Device::allocate_unique(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags flags) const {
    auto alloc_info = vk::MemoryAllocateInfo(
        requirements.size,
        this->find_memory_type(requirements.memoryTypeBits, flags).value()
    );

    return this->dev->allocateMemoryUnique(alloc_info);
}

vk::UniqueRenderPass Device::create_present_render_pass(uint32_t binding, vk::AttachmentDescription attachment) const {
    const auto attachment_ref = vk::AttachmentReference(
        binding, // The layout(location = x) of the fragment shader output
        vk::ImageLayout::eColorAttachmentOptimal
    );

    const auto subpass = vk::SubpassDescription(
        {},
        vk::PipelineBindPoint::eGraphics,
        0,
        nullptr,
        1,
        &attachment_ref
    );

    const auto dependency = vk::SubpassDependency(
        VK_SUBPASS_EXTERNAL,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::AccessFlags(),
        vk::AccessFlagBits::eColorAttachmentRead
    );

    return this->dev->createRenderPassUnique({
        {},
        1,
        &attachment,
        1,
        &subpass,
        1,
        &dependency
    });
}

vk::UniquePipeline Device::create_pipeline(
    Span<vk::PipelineShaderStageCreateInfo> shaders,
    vk::PipelineLayout pipeline_layout,
    vk::RenderPass render_pass,
    vk::Extent2D extent
) const {
    const auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo();
    const auto assembly_info = vk::PipelineInputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleStrip);
    const auto multisample_info = vk::PipelineMultisampleStateCreateInfo();

    const auto viewport = vk::Viewport(
        0.0,
        0.0,
        static_cast<float>(extent.width),
        static_cast<float>(extent.height),
        0,
        1
    );

    const auto scissor = vk::Rect2D{{0, 0}, extent};
    const auto viewport_info = vk::PipelineViewportStateCreateInfo({}, 1, &viewport, 1, &scissor);

    auto rasterizer_info = vk::PipelineRasterizationStateCreateInfo();
    rasterizer_info.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer_info.lineWidth = 1.0f;

    auto color_blend_attachment_info = vk::PipelineColorBlendAttachmentState();
    color_blend_attachment_info.colorWriteMask =
          vk::ColorComponentFlagBits::eR
        | vk::ColorComponentFlagBits::eG
        | vk::ColorComponentFlagBits::eB
        | vk::ColorComponentFlagBits::eA;

    auto color_blend_info = vk::PipelineColorBlendStateCreateInfo();
    color_blend_info.attachmentCount = 1;
    color_blend_info.pAttachments = &color_blend_attachment_info;

    return this->dev->createGraphicsPipelineUnique(vk::PipelineCache(), {
        {},
        static_cast<uint32_t>(shaders.size()),
        shaders.data(),
        &vertex_input_info,
        &assembly_info,
        nullptr,
        &viewport_info,
        &rasterizer_info,
        &multisample_info,
        nullptr,
        &color_blend_info,
        nullptr,
        pipeline_layout,
        render_pass
    });
}

vk::UniqueDescriptorSetLayout Device::create_uniform_layout(uint32_t binding) const {
    const auto output_region_layout_binding = vk::DescriptorSetLayoutBinding(
        binding,
        vk::DescriptorType::eUniformBuffer,
        1,
        vk::ShaderStageFlagBits::eFragment
    );

    return this->dev->createDescriptorSetLayoutUnique({
        {},
        1,
        &output_region_layout_binding
    });
}
