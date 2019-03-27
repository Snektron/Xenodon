#ifndef _XENODON_GRAPHICS_DEVICE_CPP
#define _XENODON_GRAPHICS_DEVICE_CPP

#include <optional>
#include <utility>
#include <cstdint>
#include <vulkan/vulkan.hpp>
#include "utility/Span.h"

struct Queue {
    vk::Queue queue;
    uint32_t family_index;
    vk::UniqueCommandPool command_pool;

    Queue(vk::Device device, uint32_t family_index);

    static Queue invalid();

    bool is_valid() const;
};

struct Device {
    vk::PhysicalDevice physical;
    vk::UniqueDevice logical;
    Queue graphics;

    Device(vk::PhysicalDevice physical, Span<const char* const> extensions, uint32_t graphics_queue);

    std::optional<uint32_t> find_memory_type(uint32_t filter, vk::MemoryPropertyFlags flags) const;

    vk::UniqueDeviceMemory allocate(vk::MemoryRequirements requirements, vk::MemoryPropertyFlags flags);

    template <typename F>
    void single_submit(Queue& queue, F f);

    template <typename F>
    void single_graphics_submit(F f);
};

template <typename F>
void Device::single_submit(Queue& queue, F f) {
    auto command_buffer_info = vk::CommandBufferAllocateInfo(queue.command_pool.get());
    command_buffer_info.commandBufferCount = 1;
    auto command_buffer = std::move(this->logical->allocateCommandBuffersUnique(command_buffer_info).front());

    auto begin_info = vk::CommandBufferBeginInfo(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    );

    command_buffer->begin(begin_info);
    f(command_buffer.get());
    command_buffer->end();

    auto submit_info = vk::SubmitInfo();
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer.get();

    queue.queue.submit(submit_info, vk::Fence());
    queue.queue.waitIdle();
}

template <typename F>
void Device::single_graphics_submit(F f) {
    this->single_submit(this->graphics, std::forward<F>(f));
}

#endif
