#include "graphics/memory/Image.h"

Image::Image(const Device& device, vk::Extent2D extent, vk::Format format, vk::ImageUsageFlags flags):
    device(device.get()) {
    this->image = device->createImage({
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
    });

    const auto reqs = device->getImageMemoryRequirements(this->image);

    this->mem = device.allocate(reqs, vk::MemoryPropertyFlagBits::eDeviceLocal);
    device->bindImageMemory(this->image, this->mem, 0);
}

Image::Image(Image&& other):
    device(other.device),
    image(other.image),
    mem(other.mem) {
    other.device = vk::Device();
    other.image = vk::Image();
    other.mem = vk::DeviceMemory();
}

Image& Image::operator=(Image&& other) {
    std::swap(this->device, other.device);
    std::swap(this->image, other.image);
    std::swap(this->mem, other.mem);
    return *this;
}

Image::~Image() {
    if (this->image != vk::Image()) {
        this->device.freeMemory(this->mem);
        this->device.destroyImage(this->image);
    }
}
