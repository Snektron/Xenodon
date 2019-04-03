#include "backend/headless/HeadlessOutput.h"
#include <cassert>
#include "core/Error.h"
#include "graphics/memory/Buffer.h"

namespace {
    constexpr const auto RENDER_TARGET_FORMAT = vk::Format::eR8G8B8A8Unorm;

    const vk::ImageUsageFlags RENDER_TARGET_USAGE = vk::ImageUsageFlagBits::eColorAttachment
        | vk::ImageUsageFlagBits::eSampled
        | vk::ImageUsageFlagBits::eTransferSrc;


    RenderDevice create_render_device(const PhysicalDevice& physdev) {
        if (auto family = physdev.find_queue_family(vk::QueueFlagBits::eGraphics)) {
            return RenderDevice(
                Device(physdev, family.value()),
                family.value(),
                1
            );
        } else {
            throw Error("Gpu does not support graphics/present queue");
        }
    }
}

HeadlessOutput::HeadlessOutput(const PhysicalDevice& physdev, vk::Rect2D render_region):
    render_region(render_region),
    rendev(create_render_device(physdev)),
    frame_fence(this->rendev.device->createFenceUnique({})),
    render_target(this->rendev.device, render_region.extent, RENDER_TARGET_FORMAT, RENDER_TARGET_USAGE) {

    auto sub_resource_range = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

    auto component_mapping = vk::ComponentMapping(
        vk::ComponentSwizzle::eR,
        vk::ComponentSwizzle::eG,
        vk::ComponentSwizzle::eB,
        vk::ComponentSwizzle::eA
    );

    auto view_create_info = vk::ImageViewCreateInfo(
        {},
        this->render_target.get(),
        vk::ImageViewType::e2D,
        RENDER_TARGET_FORMAT,
        component_mapping,
        sub_resource_range
    );

    this->render_target_view = this->rendev.device->createImageViewUnique(view_create_info);
}

uint32_t HeadlessOutput::num_swap_images() const {
    return 1;
}

uint32_t HeadlessOutput::current_swap_index() const {
    return 0;
}

SwapImage HeadlessOutput::swap_image(uint32_t index) {
    assert(index == 0);
    return SwapImage(this->render_target.get(), this->render_target_view.get(), this->frame_fence.get());
}

vk::Rect2D HeadlessOutput::region() const {
    return this->render_region;
}

vk::AttachmentDescription HeadlessOutput::color_attachment_descr() const {
    auto descr = vk::AttachmentDescription();
    descr.format = RENDER_TARGET_FORMAT;
    descr.loadOp = vk::AttachmentLoadOp::eClear;
    descr.finalLayout = vk::ImageLayout::eTransferSrcOptimal;
    return descr;
}

void HeadlessOutput::synchronize() const {
    this->rendev.device->waitForFences(this->frame_fence.get(), true, std::numeric_limits<uint64_t>::max());
    this->rendev.device->resetFences(this->frame_fence.get());
}

void HeadlessOutput::download(Pixel* output, size_t stride) {
    if (stride == 0) {
        stride = this->render_region.extent.width;
    }

    const size_t size = this->render_region.extent.width * this->render_region.extent.height;
    const auto memory_bits = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

    auto staging_buffer = Buffer<Pixel>(this->rendev.device, size, vk::BufferUsageFlagBits::eTransferDst, memory_bits);

    this->rendev.graphics_one_time_submit([this, &staging_buffer](vk::CommandBuffer cmd_buf) {
        auto copy_info = vk::BufferImageCopy(
            0,
            0,
            0,
            vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
            {0, 0, 0},
            {this->render_region.extent.width, this->render_region.extent.height, 1}
        );

        cmd_buf.copyImageToBuffer(
            this->render_target.get(),
            vk::ImageLayout::eTransferSrcOptimal,
            staging_buffer.get(),
            copy_info
        );
    });

    Pixel* pixels = staging_buffer.map(0, size);

    for (size_t y = 0; y < this->render_region.extent.height; ++y) {
        for (size_t x = 0; x < this->render_region.extent.width; ++x) {
            output[y * stride + x] = pixels[y * this->render_region.extent.width + x];
        }
    }

    staging_buffer.unmap();
}
