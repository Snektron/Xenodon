#ifndef _XENODON_UTILITY_DYNAMICARRAY_H
#define _XENODON_UTILITY_DYNAMICARRAY_H

#include <cstddef>
#include <memory>
#include <iterator>
#include <initializer_list>
#include <utility>
#include <memory>
#include <stdexcept>

template <typename T, typename Allocator = std::allocator<T>>
class DynamicArray: private Allocator {
    size_t count;
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

    explicit DynamicArray(const Allocator& alloc = Allocator()):
        Allocator(alloc),
        count(0),
        items(nullptr) {
    }

    // Requires T is default (optional) and copy constructable
    explicit DynamicArray(size_t count, const T value = T(), const Allocator& alloc = Allocator()):
        Allocator(alloc),
        count(count),
        items(this->allocate_or_null(this->count)) {

        std::uninitialized_fill(this->begin(), this->end(), value);
    }

    // Requires T is move constructable
    DynamicArray(std::initializer_list<T> init, const Allocator& alloc = Allocator()):
        Allocator(alloc),
        count(init.size()),
        items(this->allocate_or_null(this->count)) {

        std::uninitialized_move(init.begin(), init.end(), this->begin());
    }

    DynamicArray(const DynamicArray& other):
        Allocator(other.get_allocator()),
        count(other.size()),
        items(this->allocate_or_null(this->count)) {

        std::uninitialized_copy(other.begin(), other.end(), this->begin());
    }

    DynamicArray& operator=(const DynamicArray& other) {
        this->clear();
        this->get_allocator() = other.get_allocator();
        this->items = this->allocate_or_null(other.size());
        this->count = other.size();
        std::uninitialized_copy(other.begin(), other.end(), this->begin());
    }

    DynamicArray(DynamicArray&& other):
        Allocator(std::move(other.get_allocator())),
        count(other.count),
        items(other.items) {
        other.count = 0;
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
        return *this->back();
    }

    const T& back() const {
        return *this->back();
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

    void clear() {
        if (this->items != nullptr) {
            this->deallocate(this->items, this->count);
            this->count = 0;
        }
    }

    // Requires T is default (optional) and copy constructable
    void fill(size_t count, const T& value = T()) {
        this->clear();
        this->items = this->allocate_or_null(count);
        this->count = count;

        std::uninitialized_fill(this->begin(), this->end(), value);
    }

    template <typename F>
    void generate_in_place(size_t count, F f) {
        this->clear();
        this->items = this->allocate_or_null(count);
        this->count = count;

        for (size_t i = 0; i < this->count; ++i) {
            f(&this->items[i], i);
        }
    }

    template <typename... Args>
    void emplace(size_t i, Args&&... args) {
        std::destroy_at(this->items[i]);
        new (&this->items[i]) T(std::forward<Args>(args)...);
    }

    const Allocator& get_allocator() const {
        return *static_cast<const Allocator*>(this);
    }

private:
    T* allocate_or_null(size_t count) {
        return count > 0 ? this->allocate(count) : nullptr;
    }

    Allocator& get_allocator() {
        return *static_cast<Allocator*>(this);
    }
};

#endif
