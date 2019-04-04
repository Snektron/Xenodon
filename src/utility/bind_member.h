#ifndef _XENODON_UTILITY_BIND_MEMBER_H
#define _XENODON_UTILITY_BIND_MEMBER_H

template <typename T, typename R, typename... Args>
constexpr auto bind_member(T& t, R(T::*fn)(Args...)) {
    return [&t, fn](auto&&... args) {
        return (t.*fn)(std::forward<decltype(args)>(args)...);
    };
}

template <typename T, typename R, typename... Args>
constexpr auto bind_member(T* t, R(T::*fn)(Args...)) {
    return [t, fn](auto&&... args) {
        return (t->*fn)(std::forward<decltype(args)>(args)...);
    };
}

#endif
