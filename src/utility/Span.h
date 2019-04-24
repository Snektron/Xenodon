#ifndef _XENODON_UTILITY_SPAN_H
#define _XENODON_UTILITY_SPAN_H

#include <array>
#include <vector>
#include <initializer_list>
#include <type_traits>
#include <cstddef>
#include <vulkan/vulkan.hpp>

template <typename T>
class Span {
    size_t count;
    const T* ptr;

public:
    using value_type = T;
    using size_type = size_t;
    using reference = const T&;
    using const_reference = const T&;
    using pointer = const T*;
    using const_pointer = const T*;
    using iterator = const T*;
    using const_iterator = const T*;

    constexpr Span(std::nullptr_t):
        count(0),
        ptr(nullptr) {
    }

    constexpr Span(const T& ptr):
        count(1),
        ptr(&ptr) {
    }

    template <size_t N>
    constexpr Span(const T (&arr)[N]):
        count(N),
        ptr(arr) {
    }

    constexpr Span(size_t count, const T* ptr):
        count(count),
        ptr(ptr) {
    }

    template <size_t N>
    constexpr Span(const std::array<T, N>& data):
        count(N),
        ptr(data.data()) {
    }

    Span(vk::ArrayProxy<T> proxy):
        count(proxy.size()),
        ptr(proxy.data()) {
    }

    template <class Allocator>
    Span(const std::vector<T, Allocator>& data):
        count(data.size()),
        ptr(data.data()) {
    }

    constexpr const T* begin() const {
        return ptr;
    }

    constexpr const T* end() const {
        return ptr + count;
    }

    constexpr const T & front() const {
      return *this->ptr;
    }

    constexpr const T& back() const {
        return *(this->ptr + this->count - 1);
    }

    constexpr bool empty() const {
        return this->count == 0;
    }

    constexpr size_t size() const {
        return this->count;
    }

    constexpr const T* data() const {
        return this->ptr;
    }

    constexpr const T& operator[](size_t i) const {
        return this->ptr[i];
    }

    constexpr Span<T> sub(size_t begin) const {
        return Span<T>(this->count - begin, &this->ptr[begin]);
    }

    constexpr Span<T> sub(size_t begin, size_t end) const {
        return Span<T>(end - begin - 1, &this->ptr[begin]);
    }
};

#endif
