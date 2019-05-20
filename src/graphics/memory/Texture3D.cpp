#include "graphics/memory/Texture3D.h"
#include "core/Logger.h"

void Texture3D::layout_transition(vk::CommandBuffer cmd_buf, vk::PipelineStageFlags src_stage, vk::PipelineStageFlags dst_stage, const vk::ImageMemoryBarrier& barrier) {
    cmd_buf.pipelineBarrier(
        src_stage,
        dst_stage,
        vk::DependencyFlags(),
        nullptr,
        nullptr,
        barrier
    );
}

Texture3D::Texture3D(Texture3D&& other):
    dev(other.dev),
    image(other.image),
    mem(other.mem),
    image_view(other.image_view) {
    other.dev = nullptr;
    other.image = vk::Image();
    other.mem = vk::DeviceMemory();
    other.image_view = vk::ImageView();
}

Texture3D& Texture3D::operator=(Texture3D&& other) {
    std::swap(this->dev, other.dev);
    std::swap(this->image, other.image);
    std::swap(this->mem, other.mem);
    std::swap(this->image_view, other.image_view);
    return *this;
}

Texture3D::~Texture3D() {
    if (this->image != vk::Image()) {
        this->dev->get().destroyImageView(this->image_view);
        this->dev->get().freeMemory(this->mem);
        this->dev->get().destroyImage(this->image);
    }
}

Texture3D::Texture3D(const Device& dev, vk::Format format, vk::Extent3D extent, vk::ImageUsageFlags flags):
    dev(&dev) {
    this->image = dev->createImage({
        {},
        vk::ImageType::e3D,
        format,
        extent,
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        flags,
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        vk::ImageLayout::eUndefined
    });

    const auto reqs = dev->getImageMemoryRequirements(this->image);

    this->mem = dev.allocate(reqs, vk::MemoryPropertyFlagBits::eDeviceLocal);
    dev->bindImageMemory(this->image, this->mem, 0);

    auto sub_resource_range = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

    auto component_mapping = vk::ComponentMapping(
        vk::ComponentSwizzle::eR,
        vk::ComponentSwizzle::eG,
        vk::ComponentSwizzle::eB,
        vk::ComponentSwizzle::eA
    );

    this->image_view = dev->createImageView({
        {},
        this->image,
        vk::ImageViewType::e3D,
        format,
        component_mapping,
        sub_resource_range
    });
}
