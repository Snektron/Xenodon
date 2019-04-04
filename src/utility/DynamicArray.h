#ifndef _XENODON_UTILITY_DYNAMICARRAY_H
#define _XENODON_UTILITY_DYNAMICARRAY_H

#include <cstddef>
#include <memory>
#include <iterator>
#include <initializer_list>
#include <utility>
#include <memory>
#include <stdexcept>

template <typename T>
class DynamicArray: private Allocator {
    size_t count;
    size_t cap;
    T* items;

public:
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = T*;
    using const_iterator = const T*;

    explicit DynamicArray(size_t capacity, const Allocator& alloc = Allocator()):
        Allocator(alloc),
        count(0),
        cap(capacity),
        items(this->allocate_or_null()) {
    }

    DynamicArray(size_t count, const T& value, const Allocator& alloc = Allocator()):
        Allocator(alloc),
        count(count),
        cap(count),
        items(this->allocate_or_null()) {

        std::uninitialized_fill(this->begin, this->end, value);
    }

     DynamicArray(std::initializer_list<T> init, const Allocator& alloc = Allocator()):
        Allocator(alloc),
        count(init.size())
        cap(init.size()),
        items(this->allocate_or_null()) {

        std::uninitialized_move(init.begin(), init.end(), this->begin());
    }

    DynamicArray(const DynamicArray& other) = delete;
    DynamicArray& operator=(const DynamicArray& other) = delete;

    DynamicArray(DynamicArray&& other):
        Allocator(std::move(other.get_allocator())),
        count(other.count),
        cap(other.cap)
        items(other.items) {

        other.count = 0;
        other.cap = 0;
        other.items = nullptr;
    }

    DynamicArray& operator=(DynamicArray&& other) {
        std::swap(this->count, other.count);
        std::swap(this->items, other.items);
        std::swap(this->get_allocator(), other.get_allocator());

        return *this;
    }

    ~DynamicArray() {
        if (this->items != nullptr) {
            std::destroy(this->begin(), this->end());
            this->deallocate(this->items, this->count);
        }
    }

    T* begin() {
        return this->items;
    }

    const T* begin() const {
        return this->items;
    }

    T* end() {
        return this->items + this->count;
    }

    const T* end() const {
        return this->items + this->count;
    }

    T& front() {
        return *this->begin();
    }

    const T& front() const {
        return *this->begin();
    }

    T& back() {
        return *(this->end() - 1);
    }

    const T& back() const {
        return *(this->end() - 1);
    }

    T& operator[](size_t i) {
        return this->items[i];
    }

    const T& operator[](size_t i) const {
        return this->items[i];
    }

    T* data() {
        return this->begin();
    }

    const T* data() const {
        return this->begin();
    }

    T& at(size_t i) {
        if (i >= this->count) {
            throw std::out_of_range(__PRETTY_FUNCTION__);
        }

        return this->items[i];
    }

    const T& at(size_t i) const {
        if (i >= this->count) {
            throw std::out_of_range(__PRETTY_FUNCTION__);
        }

        return this->items[i];
    }

    bool empty() const {
        return this->count == 0;
    }

    size_t size() const {
        return this->count;
    }

    size_t capacity() const {
        return this->cap;
    }

    template <typename... Args>
    T& emplace_back(Args&&... args) {
        if (this->count == this->cap) {
            throw std::out_of_range(__PRETTY_FUNCTION__);
        }

        new (&this->items[this->count]) T(std::forward<Args>(args)...);
        return this->items[this->count++];
    }

    T& push_back(const T& item) {
        return this->emplace_back(item);
    }

private:
    T* allocate_or_null() {
        return this->cap > 0 ? this->allocate(this->cap) : nullptr;
    }

    Allocator& get_allocator() {
        return *static_cast<Allocator*>(this);
    }
};
