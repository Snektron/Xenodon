#ifndef _XENODON_GRAPHICS_MEMORY_BUFFER_H
#define _XENODON_GRAPHICS_MEMORY_BUFFER_H

#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "graphics/core/Device.h"

template <typename T>
struct Buffer {
    vk::Device device;
    vk::Buffer buffer;
    vk::DeviceMemory mem;

    Buffer(const Device& device, vk::DeviceSize elements, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags memory_flags);

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&&);
    Buffer& operator=(Buffer&&);

    ~Buffer();

    T* map(vk::DeviceSize offset, vk::DeviceSize size);
    void unmap();

    void update_descriptor_write(vk::DescriptorSet set, size_t index);

    vk::Buffer get() const {
        return this->buffer;
    }

    vk::DeviceMemory memory() const {
        return this->mem;
    }
};

template <typename T>
Buffer<T>::Buffer(const Device& device, vk::DeviceSize elements, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags memory_flags):
    device(device.get()) {
    auto buffer_create_info = vk::BufferCreateInfo(
        {},
        static_cast<vk::DeviceSize>(elements * sizeof(T)),
        usage_flags
    );

    this->buffer = device->createBuffer(buffer_create_info);
    const auto reqs = device->getBufferMemoryRequirements(this->buffer);
    this->mem = device.allocate(reqs, memory_flags);
    device->bindBufferMemory(this->buffer, this->mem, 0);
}

template <typename T>
Buffer<T>::Buffer(Buffer&& other):
    device(other.device),
    buffer(other.buffer),
    mem(other.mem) {
    other.device = vk::Device();
    other.buffer = vk::Buffer();
    other.mem = vk::DeviceMemory();
}

template <typename T>
Buffer<T>& Buffer<T>::operator=(Buffer&& other) {
    std::swap(this->device, other.device);
    std::swap(this->buffer, other.buffer);
    std::swap(this->mem, other.mem);
    return *this;
}

template <typename T>
Buffer<T>::~Buffer() {
    if (this->buffer != vk::Buffer()) {
        this->device.freeMemory(this->mem);
        this->device.destroyBuffer(this->buffer);
    }
}

template <typename T>
T* Buffer<T>::map(vk::DeviceSize offset, vk::DeviceSize size) {
    return reinterpret_cast<T*>(this->device.mapMemory(this->mem, offset * sizeof(T), size * sizeof(T)));
}

template <typename T>
void Buffer<T>::unmap() {
    this->device.unmapMemory(this->mem);
}

template <typename T>
void Buffer<T>::update_descriptor_write(vk::DescriptorSet set, size_t index) {
    const auto buffer_info = vk::DescriptorBufferInfo(
        this->buffer,
        index * sizeof(T),
        sizeof(T)
    );

    const auto descriptor_write = vk::WriteDescriptorSet(
        set,
        0,
        0,
        1,
        vk::DescriptorType::eUniformBuffer,
        nullptr,
        &buffer_info,
        nullptr
    );

    this->device.updateDescriptorSets(descriptor_write, nullptr);
}

#endif
