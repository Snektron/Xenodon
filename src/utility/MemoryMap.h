#ifndef _XENODON_UTILITY_MEMORYMAP_H
#define _XENODON_UTILITY_MEMORYMAP_H

#include <cstddef>
#include "utility/Span.h"

class MemoryMap {
    size_t size;
    std::byte* memory;

public:
    MemoryMap(const char* path);
    ~MemoryMap();

    Span<std::byte> data() const {
        return Span(this->size, this->memory);
    }
};

#endif
