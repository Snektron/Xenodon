#include "interactive/Display.h"
#include <utility>
#include <limits>
#include <stdexcept>
#include <array>

namespace {
    constexpr const size_t MAX_FRAMES = 2;
}

Display::FrameSync::FrameSync(vk::Device device):
    image_available(device.createSemaphoreUnique(vk::SemaphoreCreateInfo())),
    render_finished(device.createSemaphoreUnique(vk::SemaphoreCreateInfo())),
    fence(device.createFenceUnique(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled))) {
}

Display::Display(DeviceContext&& device_context, Window&& window, vk::UniqueSurfaceKHR&& surface, vk::Rect2D area):
    device_context(std::move(device_context)),
    window(std::move(window)),
    surface(std::move(surface)),
    sinf(this->device_context.physical, this->surface.get(), area.extent),
    area(area.offset, this->sinf.extent),
    renderer(std::make_unique<RenderWorker>(this->device_context, area, this->sinf.attachment_description)),
    swapchain(this->device_context, this->surface.get(), this->sinf, this->renderer->final_render_pass()),
    current_frame(0) {

    this->sync_objects.reserve(MAX_FRAMES);
    for (size_t i = 0; i < MAX_FRAMES; ++i) {
        this->sync_objects.emplace_back(this->device_context.logical.get());
    }
}

Display::~Display() {
    this->device_context.logical->waitIdle();
}

void Display::reconfigure(vk::Rect2D area) {
    this->area.offset = area.offset;
    if (area.extent == this->area.extent)
        return;

    this->recreate_swapchain(area.extent);
}

void Display::recreate_swapchain(vk::Extent2D window_extent) {
    this->device_context.logical->waitIdle();
    this->sinf = SurfaceInfo(device_context.physical, this->surface.get(), window_extent);
    this->area.extent = this->sinf.extent;
    this->renderer = std::make_unique<RenderWorker>(this->device_context, this->area, this->sinf.attachment_description);
    this->swapchain.recreate(this->sinf, this->renderer->final_render_pass());
}

void Display::present() {
    auto& device = this->device_context.logical.get();

    const auto fence = this->sync_objects[this->current_frame].fence.get();
    device.waitForFences(fence, true, std::numeric_limits<uint64_t>::max());
    device.resetFences(fence);

    const auto image_available = this->sync_objects[this->current_frame].image_available.get();
    const auto render_finished = this->sync_objects[this->current_frame].render_finished.get();

    vk::Result result = this->swapchain.acquire_next_image(image_available);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        this->recreate_swapchain(this->area.extent);
        result = this->swapchain.acquire_next_image(image_available);
    }

    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to acquire next image: " + vk::to_string(result));
    }

    renderer->present(this->swapchain.active_frame().command_buffer.get(), this->swapchain.active_frame().framebuffer.get());

    auto wait_stages = std::array{vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput)};
    auto submit_info = vk::SubmitInfo(
        1,
        &image_available,
        wait_stages.data(),
        1,
        &this->swapchain.active_frame().command_buffer.get(),
        1,
        &render_finished
    );

    this->device_context.graphics.queue.submit(1, &submit_info, fence);
    this->swapchain.present(render_finished);

    current_frame = (current_frame + 1) % MAX_FRAMES;
}
