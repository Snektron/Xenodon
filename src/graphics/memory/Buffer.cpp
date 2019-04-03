#include "graphics/memory/Buffer.h"

Buffer2::Buffer2(const Device2& device, vk::DeviceSize size, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags memory_flags) {
    auto buffer_create_info = vk::BufferCreateInfo(
        {},
        size,
        usage_flags
    );

    this->buffer = device->createBufferUnique(buffer_create_info);
    auto reqs = device->getBufferMemoryRequirements(this->get());
    this->mem = device.allocate(reqs, memory_flags);
    device->bindBufferMemory(this->get(), this->memory(), 0);
}
