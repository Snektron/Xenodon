#ifndef _XENODON_GRAPHICS_MEMORY_TEXTURE3D_H
#define _XENODON_GRAPHICS_MEMORY_TEXTURE3D_H

#include "graphics/core/Device.h"
#include "graphics/memory/Buffer.h"
#include "utility/Span.h"

class Texture3D {
    const Device* device;
    vk::Image image;
    vk::DeviceMemory mem;
    vk::ImageView image_view;

public:
    static void layout_transition(vk::CommandBuffer cmd_buf, vk::PipelineStageFlags src_stage, vk::PipelineStageFlags dst_stage, const vk::ImageMemoryBarrier& barrier);

    Texture3D(const Device& device, vk::Format format, vk::Extent3D extent, vk::ImageUsageFlags flags);

    Texture3D(const Texture3D&) = delete;
    Texture3D& operator=(const Texture3D&) = delete;

    Texture3D(Texture3D&& other);
    Texture3D& operator=(Texture3D&& other);

    ~Texture3D();

    template <typename T>
    auto upload(Span<T> data, const vk::BufferImageCopy& region, vk::ImageLayout dst_layout);

    vk::Image get() const {
        return this->image;
    }

    vk::DeviceMemory memory() const {
        return this->mem;
    }

    vk::ImageView view() const {
        return this->image_view;
    }
};

template <typename T>
auto Texture3D::upload(Span<T> data, const vk::BufferImageCopy& region, vk::ImageLayout dst_layout) {
    auto staging_buffer = Buffer<T>(
        *this->device,
        data.size(),
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );

    return [this, staging_buffer = std::move(staging_buffer), data, &region, dst_layout](vk::CommandBuffer cmd_buf) {
        auto* mapping = staging_buffer.map(0, data.size());

        for (size_t i = 0; i < data.size(); ++i) {
            mapping[i] = data[i];
        }

        staging_buffer.unmap();

        cmd_buf.copyBufferToImage(
            staging_buffer.get(),
            this->image,
            dst_layout,
            region
        );
    };
}

#endif
