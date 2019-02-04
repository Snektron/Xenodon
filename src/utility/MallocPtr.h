#ifndef _XENODON_UTILITY_MALLOCPTR_H
#define _XENODON_UTILITY_MALLOCPTR_H

#include <memory>
#include <cstdlib>

template <typename T>
struct Free {
    void operator()(T* ptr) {
        std::free(static_cast<void*>(ptr));
    }
};

template <typename T>
using MallocPtr = std::unique_ptr<T, Free<T>>;

#endif
