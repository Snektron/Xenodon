#ifndef _XENODON_GRAPHICS_COMMAND_COMMANDPOOL_H
#define _XENODON_GRAPHICS_COMMAND_COMMANDPOOL_H

#include <vector>
#include <vulkan/vulkan.hpp>
#include "graphics/core/Device.h"
#include "graphics/core/Queue.h"

class CommandPool {
    vk::Device device;
    vk::Queue queue;
    vk::CommandPool pool;

public:
    CommandPool(const Device& device, const Queue& queue);

    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;

    CommandPool(CommandPool&& other);
    CommandPool& operator=(CommandPool&& other);

    ~CommandPool();

    vk::UniqueCommandBuffer allocate_command_buffer() const;
    std::vector<vk::UniqueCommandBuffer> allocate_command_buffers(size_t num) const;

    template <typename F>
    void one_time_submit(F f) const;

    vk::CommandPool get() const {
        return this->pool;
    }
};

template <typename F>
void CommandPool::one_time_submit(F f) const {
    auto cmdbuf = this->allocate_command_buffer();

    cmdbuf->begin({
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    });

    f(cmdbuf.get());

    cmdbuf->end();

    auto submit_info = vk::SubmitInfo();
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmdbuf.get();

    this->queue.submit(submit_info, vk::Fence());
    this->queue.waitIdle();
}

#endif
