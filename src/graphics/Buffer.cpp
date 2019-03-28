#include "graphics/Buffer.h"

Buffer::Buffer(Device& device, vk::DeviceSize size, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags memory_flags) {
    auto buffer_create_info = vk::BufferCreateInfo(
        {},
        size,
        usage_flags
    );

    this->buf = device.logical->createBufferUnique(buffer_create_info);
    auto reqs = device.logical->getBufferMemoryRequirements(this->buffer());
    this->mem = device.allocate(reqs, memory_flags);
    device.logical->bindBufferMemory(this->buffer(), this->memory(), 0);
}
