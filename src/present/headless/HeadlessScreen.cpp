#include "present/headless/HeadlessScreen.h"
#include <cstring>
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
    render_target(this->device, RENDER_TARGET_FORMAT, render_region.extent) {

    auto command_buffer_info = vk::CommandBufferAllocateInfo(this->device.graphics.command_pool.get());
    command_buffer_info.commandBufferCount = 1;
    this->command_buffer = std::move(this->device.logical->allocateCommandBuffersUnique(command_buffer_info).front());
}

uint32_t HeadlessScreen::num_swap_images() const {
    return 1;
}

SwapImage HeadlessScreen::swap_image(uint32_t index) const {
    return {
        this->command_buffer.get(),
        this->render_target.image.get(),
        this->render_target.view.get()
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

std::vector<Pixel> HeadlessScreen::download() {
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
            this->render_target.image.get(),
            vk::ImageLayout::eTransferSrcOptimal,
            staging_buffer.buffer.get(),
            copy_info
        );
    });

    auto pixels = std::vector<Pixel>();
    pixels.resize(n_pixels);

    Pixel* staging_pixels = reinterpret_cast<Pixel*>(this->device.logical->mapMemory(staging_buffer.memory.get(), 0, size, {}));

    std::memcpy(pixels.data(), staging_pixels, size);

    this->device.logical->unmapMemory(staging_buffer.memory.get());

    return pixels;
}