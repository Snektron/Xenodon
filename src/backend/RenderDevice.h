#ifndef _XENODON_BACKEND_RENDERDEVICE_H
#define _XENODON_BACKEND_RENDERDEVICE_H

#include <utility>
#include <cstdint>
#include "graphics/core/Device.h"
#include "graphics/core/Queue.h"
#include "backend/Output.h"
#include "utility/Span.h"

struct RenderDevice {
    Device2 device;
    Queue2 graphics_queue;
    size_t outputs;
    vk::UniqueCommandPool graphics_command_pool;

    RenderDevice(Device2&& device, uint32_t graphics_queue_family, size_t outputs);

    template <typename F>
    void one_time_submit(Queue2& queue, vk::CommandPool pool, F f);

    template <typename F>
    void graphics_one_time_submit(F&& f);

    vk::CommandPool graphics_pool() const {
        return this->graphics_command_pool.get();
    }
};

template <typename F>
void RenderDevice::one_time_submit(Queue2& queue, vk::CommandPool pool, F f) {
    auto command_buffer_info = vk::CommandBufferAllocateInfo(pool);
    command_buffer_info.commandBufferCount = 1;
    auto command_buffer = std::move(this->device->allocateCommandBuffersUnique(command_buffer_info).front());

    auto begin_info = vk::CommandBufferBeginInfo(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    );

    command_buffer->begin(begin_info);
    f(command_buffer.get());
    command_buffer->end();

    auto submit_info = vk::SubmitInfo();
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer.get();

    queue->submit(submit_info, vk::Fence());
    queue->waitIdle();
}

template <typename F>
void RenderDevice::graphics_one_time_submit(F&& f) {
    this->one_time_submit(this->graphics_queue, this->graphics_pool(), std::forward<F>(f));
}

#endif
