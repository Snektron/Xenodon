#include "graphics/Image.h"

const vk::ImageUsageFlags Image::DEFAULT_USAGE = vk::ImageUsageFlagBits::eSampled;

Image::Image(Device& device, vk::Extent2D extent, vk::Format format, vk::ImageUsageFlags flags) {
    auto image_create_info = vk::ImageCreateInfo(
        {},
        vk::ImageType::e2D,
        format,
        {extent.width, extent.height, 1},
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        flags,
        vk::SharingMode::eExclusive
    );

    this->img = device.logical->createImageUnique(image_create_info);
    auto reqs = device.logical->getImageMemoryRequirements(this->img.get());
    this->mem = device.allocate(reqs, vk::MemoryPropertyFlagBits::eDeviceLocal);
    device.logical->bindImageMemory(this->img.get(), this->mem.get(), 0);
}
