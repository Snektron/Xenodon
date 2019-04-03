#include "backend/SwapImage.h"

SwapImage2::SwapImage2(const Swapchain2::SwapImageResources& resources):
    image(resources.image),
    view(resources.view.get()),
    image_acquired(resources.image_acquired.get()),
    render_finished(resources.render_finished.get()),
    frame_fence(resources.frame_fence.get()) {
}

SwapImage2::SwapImage2(vk::Image image, vk::ImageView view, vk::Fence frame_fence):
    image(image),
    view(view),
    frame_fence(frame_fence) {
}

void SwapImage2::submit(Queue2 queue, vk::CommandBuffer cmd_buf, vk::PipelineStageFlags flags) const {
    auto submit_info = vk::SubmitInfo(
        static_cast<uint32_t>(this->image_acquired != vk::Semaphore()),
        &this->image_acquired,
        &flags,
        1,
        &cmd_buf,
        static_cast<uint32_t>(this->render_finished != vk::Semaphore()),
        &this->render_finished
    );

    queue->submit(1, &submit_info, this->frame_fence);
}
