#include "graphics/command/CommandPool.h"

CommandPool::CommandPool(const Device& device, const Queue& queue):
    device(device.get()),
    queue(queue.get()) {

    this->pool = device->createCommandPool({
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        queue.queue_family_index()
    });
}

CommandPool::CommandPool(CommandPool&& other):
    device(other.device),
    queue(other.queue),
    pool(other.pool) {
    other.device = vk::Device();
    other.queue = vk::Queue();
    other.pool = vk::CommandPool();
}

CommandPool& CommandPool::operator=(CommandPool&& other) {
    std::swap(this->device, other.device);
    std::swap(this->queue, other.queue);
    std::swap(this->pool, other.pool);
    return *this;
}

CommandPool::~CommandPool() {
    if (this->pool != vk::CommandPool()) {
        this->device.destroyCommandPool(this->pool);
    }
}

vk::UniqueCommandBuffer CommandPool::allocate_command_buffer() const {
    auto buf = this->allocate_command_buffers(1);
    return std::move(buf[0]);
}

std::vector<vk::UniqueCommandBuffer> CommandPool::allocate_command_buffers(size_t num) const {
    return this->device.allocateCommandBuffersUnique({
        this->pool,
        vk::CommandBufferLevel::ePrimary,
        static_cast<uint32_t>(num)
    });
}
