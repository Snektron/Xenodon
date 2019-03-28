#include "graphics/RenderTarget.h"

const vk::ImageUsageFlags RenderTarget::USAGE_FLAGS =
      vk::ImageUsageFlagBits::eColorAttachment
    | vk::ImageUsageFlagBits::eSampled
    | vk::ImageUsageFlagBits::eTransferSrc;

RenderTarget::RenderTarget(Device& device, vk::Extent2D extent, vk::Format format):
    img(device, extent, format, USAGE_FLAGS) {
    auto sub_resource_range = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

    auto component_mapping = vk::ComponentMapping(
        vk::ComponentSwizzle::eR,
        vk::ComponentSwizzle::eG,
        vk::ComponentSwizzle::eB,
        vk::ComponentSwizzle::eA
    );

    auto view_create_info = vk::ImageViewCreateInfo(
        {},
        this->image(),
        vk::ImageViewType::e2D,
        format,
        component_mapping,
        sub_resource_range
    );

    this->img_view = device.logical->createImageViewUnique(view_create_info);
}
