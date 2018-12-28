#ifndef _XENODON_UTILITY_SCOPEGUARD_H
#define _XENODON_UTILITY_SCOPEGUARD_H

#include <utility>

template <typename F>
struct ScopeGuard {
    F f;

    ScopeGuard(F&& f):
        f(std::forward<F>(f)) {
    }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&&) = default;

    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = default;

    ~ScopeGuard() {
        this->f();
    }
};

#endif
