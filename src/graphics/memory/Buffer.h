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

    T* map(vk::DeviceSize offset, vk::DeviceSize size) const;
    void unmap() const;

    void update_descriptor_write(vk::DescriptorSet set, uint32_t binding, size_t index);

    vk::DescriptorBufferInfo descriptor_info(size_t offset, size_t size) const;

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
T* Buffer<T>::map(vk::DeviceSize offset, vk::DeviceSize size) const {
    return reinterpret_cast<T*>(this->device.mapMemory(this->mem, offset * sizeof(T), size * sizeof(T)));
}

template <typename T>
void Buffer<T>::unmap() const {
    this->device.unmapMemory(this->mem);
}

template <typename T>
vk::DescriptorBufferInfo Buffer<T>::descriptor_info(size_t offset, size_t size) const {
    return vk::DescriptorBufferInfo(
        this->buffer,
        offset * sizeof(T),
        size * sizeof(T)
    );
}

#endif
