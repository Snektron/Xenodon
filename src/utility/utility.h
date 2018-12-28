#ifndef _XENODON_UTILITY_UTILITY_H
#define _XENODON_UTILITY_UTILITY_H

#include <utility>

template <typename F>
struct Defer {
    F f;

    Defer(F&& f):
        f(std::forward<F>(f)) {
    }

    ~Defer() {
        this->f();
    }
};

#endif
