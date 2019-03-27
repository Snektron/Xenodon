#include "graphics/Buffer.h"

Buffer::Buffer(Device& device, vk::DeviceSize size, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags memory_flags) {
    auto buffer_create_info = vk::BufferCreateInfo(
        {},
        size,
        usage_flags
    );

    this->buffer = device.logical->createBufferUnique(buffer_create_info);
    auto reqs = device.logical->getBufferMemoryRequirements(this->buffer.get());
    this->memory = device.allocate(reqs, memory_flags);
    device.logical->bindBufferMemory(this->buffer.get(), this->memory.get(), 0);
}
