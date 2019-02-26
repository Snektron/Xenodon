#ifndef _XENODON_UTILITY_BIND_MEMBER_H
#define _XENODON_UTILITY_BIND_MEMBER_H

template <typename T, typename R, typename... Args>
auto bind_member(T& t, R(T::*fn)(Args...)) {
    return [&t, fn](Args&&... args) {
        return (t.*fn)(std::forward<Args>(args)...);
    };
}

template <typename T, typename R, typename... Args>
auto bind_member(T* t, R(T::*fn)(Args...)) {
    return [t, fn](Args&&... args) {
        return (t->*fn)(std::forward<Args>(args)...);
    };
}

#endif
