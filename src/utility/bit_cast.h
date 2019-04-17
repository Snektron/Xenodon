#ifndef _XENODON_UTILITY_BIT_CAST_H
#define _XENODON_UTILITY_BIT_CAST_H

#include <type_traits>
#include <cstring>

// std::bit_cast is sadly c++20, so just include the example implementation
// https://en.cppreference.com/w/cpp/numeric/bit_cast

template <
    typename To,
    typename From,
    typename = std::enable_if_t<
        (sizeof(To) == sizeof(From)) &&
        std::is_trivially_copyable_v<From> &&
        std::is_trivial_v<To>,
    >
>
To bit_cast(const From& src) {
    To dst;
    std::memcpy(&dst, &src, sizeof(To));
    return dst;
}

#endif
