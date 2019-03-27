#include "graphics/RenderTarget.h"

RenderTarget::RenderTarget(Device& device, vk::Format format, vk::Extent2D extent) {
    auto image_create_info = vk::ImageCreateInfo(
        {},
        vk::ImageType::e2D,
        format,
        {extent.width, extent.height, 1},
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
        vk::SharingMode::eExclusive
    );

    this->image = device.logical->createImageUnique(image_create_info);
    auto reqs = device.logical->getImageMemoryRequirements(this->image.get());
    this->memory = device.allocate(reqs, vk::MemoryPropertyFlagBits::eDeviceLocal);
    device.logical->bindImageMemory(this->image.get(), this->memory.get(), 0);

    auto sub_resource_range = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

    auto component_mapping = vk::ComponentMapping(
        vk::ComponentSwizzle::eR,
        vk::ComponentSwizzle::eG,
        vk::ComponentSwizzle::eB,
        vk::ComponentSwizzle::eA
    );

    auto view_create_info = vk::ImageViewCreateInfo(
        {},
        this->image.get(),
        vk::ImageViewType::e2D,
        format,
        component_mapping,
        sub_resource_range
    );

    this->view = device.logical->createImageViewUnique(view_create_info);
}
