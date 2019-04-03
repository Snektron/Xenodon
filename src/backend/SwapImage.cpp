#include "backend/SwapImage.h"

SwapImage::SwapImage(const Swapchain::SwapImageResources& resources):
    image(resources.image),
    view(resources.view.get()),
    image_acquired(resources.image_acquired.get()),
    render_finished(resources.render_finished.get()),
    frame_fence(resources.frame_fence.get()) {
}

SwapImage::SwapImage(vk::Image image, vk::ImageView view, vk::Fence frame_fence):
    image(image),
    view(view),
    frame_fence(frame_fence) {
}

void SwapImage::submit(Queue queue, vk::CommandBuffer cmd_buf, vk::PipelineStageFlags flags) const {
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
