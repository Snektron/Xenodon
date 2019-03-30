#include "present/headless/HeadlessScreen.h"
#include "graphics/utility.h"
#include "graphics/Swapchain.h"
#include "graphics/Buffer.h"

namespace {
    constexpr const auto RENDER_TARGET_FORMAT = vk::Format::eR8G8B8A8Unorm;

    Device create_device(vk::PhysicalDevice gpu) {
        if (auto queue = pick_graphics_queue(gpu, nullptr)) {
            return Device(gpu, nullptr, queue.value());
        } else {
            throw std::runtime_error("Gpu does not support graphics/present queue");
        }
    }
}

HeadlessScreen::HeadlessScreen(vk::PhysicalDevice gpu, vk::Rect2D render_region):
    render_region(render_region),
    device(create_device(gpu)),
    render_target(this->device, render_region.extent, RENDER_TARGET_FORMAT) {

    auto command_buffer_info = vk::CommandBufferAllocateInfo(this->device.graphics.command_pool.get());
    command_buffer_info.commandBufferCount = 1;
    this->command_buffer = std::move(this->device.logical->allocateCommandBuffersUnique(command_buffer_info).front());
}

uint32_t HeadlessScreen::num_swap_images() const {
    return 1;
}

SwapImage HeadlessScreen::swap_image([[maybe_unused]] uint32_t index) const {
    return {
        this->command_buffer.get(),
        this->render_target.image(),
        this->render_target.view()
    };
}

vk::Result HeadlessScreen::present(Swapchain::PresentCallback f) {
    f(0, this->swap_image(0));

    auto wait_stages = std::array{
        vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput)
    };

    auto submit_info = vk::SubmitInfo(
        0,
        nullptr,
        wait_stages.data(),
        1,
        &this->command_buffer.get(),
        0,
        nullptr
    );

    this->device.graphics.queue.submit(1, &submit_info, vk::Fence());

    return vk::Result::eSuccess;
}

vk::Rect2D HeadlessScreen::region() const {
    return this->render_region;
}

vk::AttachmentDescription HeadlessScreen::color_attachment_descr() const {
    auto descr = vk::AttachmentDescription();
    descr.format = RENDER_TARGET_FORMAT;
    descr.loadOp = vk::AttachmentLoadOp::eClear;
    descr.finalLayout = vk::ImageLayout::eTransferSrcOptimal;
    return descr;
}

void HeadlessScreen::download(Pixel* pixels, size_t stride) {
    const size_t n_pixels = this->render_region.extent.width * this->render_region.extent.height;
    const size_t size = n_pixels * sizeof(Pixel);
    const auto memory_bits = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

    auto staging_buffer = Buffer(this->device, size, vk::BufferUsageFlagBits::eTransferDst, memory_bits);

    this->device.single_graphics_submit([this, &staging_buffer](vk::CommandBuffer cmd_buf) {
        auto copy_info = vk::BufferImageCopy(
            0,
            0,
            0,
            vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
            {0, 0, 0},
            {this->render_region.extent.width, this->render_region.extent.height, 1}
        );

        cmd_buf.copyImageToBuffer(
            this->render_target.image(),
            vk::ImageLayout::eTransferSrcOptimal,
            staging_buffer.buffer(),
            copy_info
        );
    });

    Pixel* staging_pixels = reinterpret_cast<Pixel*>(this->device.logical->mapMemory(staging_buffer.memory(), 0, size));

    if (stride == 0) {
        stride = this->render_region.extent.width;
    }

    for (size_t y = 0; y < this->render_region.extent.height; ++y) {
        for (size_t x = 0; x < this->render_region.extent.width; ++x) {
            pixels[y * stride + x] = staging_pixels[y * this->render_region.extent.width + x];
        }
    }

    this->device.logical->unmapMemory(staging_buffer.memory());
}