#ifndef _XENODON_GRAPHICS_MEMORY_BUFFER_H
#define _XENODON_GRAPHICS_MEMORY_BUFFER_H

#include <cstddef>
#include <vulkan/vulkan.hpp>
#include "graphics/core/Device.h"

template <typename T>
struct Buffer {
    vk::Device dev;
    vk::Buffer buffer;
    vk::DeviceMemory mem;

    Buffer(const Device& dev, vk::DeviceSize elements, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags memory_flags);

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&&);
    Buffer& operator=(Buffer&&);

    ~Buffer();

    T* map(vk::DeviceSize offset, vk::DeviceSize size) const;
    void unmap() const;

    vk::DescriptorBufferInfo descriptor_info(size_t offset, size_t size) const;

    vk::Buffer get() const {
        return this->buffer;
    }

    vk::DeviceMemory memory() const {
        return this->mem;
    }

    vk::Device device() const {
        return this->dev;
    }
};

template <typename T>
Buffer<T>::Buffer(const Device& dev, vk::DeviceSize elements, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags memory_flags):
    dev(dev.get()) {
    auto buffer_create_info = vk::BufferCreateInfo(
        {},
        static_cast<vk::DeviceSize>(elements * sizeof(T)),
        usage_flags
    );

    this->buffer = dev->createBuffer(buffer_create_info);
    const auto reqs = dev->getBufferMemoryRequirements(this->buffer);
    this->mem = dev.allocate(reqs, memory_flags);
    dev->bindBufferMemory(this->buffer, this->mem, 0);
}

template <typename T>
Buffer<T>::Buffer(Buffer&& other):
    dev(other.dev),
    buffer(other.buffer),
    mem(other.mem) {
    other.dev = vk::Device();
    other.buffer = vk::Buffer();
    other.mem = vk::DeviceMemory();
}

template <typename T>
Buffer<T>& Buffer<T>::operator=(Buffer&& other) {
    std::swap(this->dev, other.dev);
    std::swap(this->buffer, other.buffer);
    std::swap(this->mem, other.mem);
    return *this;
}

template <typename T>
Buffer<T>::~Buffer() {
    if (this->buffer != vk::Buffer()) {
        this->dev.freeMemory(this->mem);
        this->dev.destroyBuffer(this->buffer);
    }
}

template <typename T>
T* Buffer<T>::map(vk::DeviceSize offset, vk::DeviceSize size) const {
    return reinterpret_cast<T*>(this->dev.mapMemory(this->mem, offset * sizeof(T), size * sizeof(T)));
}

template <typename T>
void Buffer<T>::unmap() const {
    this->dev.unmapMemory(this->mem);
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
