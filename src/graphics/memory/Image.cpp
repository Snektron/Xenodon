#include "graphics/memory/Image.h"

Image::Image(const Device& device, vk::Extent2D extent, vk::Format format, vk::ImageUsageFlags flags) {
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

    this->image = device->createImageUnique(image_create_info);
    auto reqs = device->getImageMemoryRequirements(this->image.get());
    this->mem = device.allocate(reqs, vk::MemoryPropertyFlagBits::eDeviceLocal);
    device->bindImageMemory(this->image.get(), this->mem.get(), 0);
}
