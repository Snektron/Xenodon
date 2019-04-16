#ifndef _XENODON_MODEL_STORAGE_H
#define _XENODON_MODEL_STORAGE_H

#include <vector>
#include "utility/Span.h"

template <typename T>
struct Storage {
    virtual ~Storage() = default;
    virtual Span<const T> data() = 0;
};

#endif
