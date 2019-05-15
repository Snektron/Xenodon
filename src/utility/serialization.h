#ifndef _XENODON_UTILITY_SERIALIZATION_H
#define _XENODON_UTILITY_SERIALIZATION_H

#include <limits>
#include <ostream>
#include <istream>

// These functions assume CHAR_BIT == 8

template <typename T>
T read_uint_le(std::istream& in) {
    static_assert(std::numeric_limits<T>::is_integer && !std::numeric_limits<T>::is_signed, "read_uint only read an unsigned integer");
    constexpr const size_t value_size = sizeof(T);

    uint8_t data[value_size];
    in.read(reinterpret_cast<char*>(data), value_size);

    T value = static_cast<T>(0);
    for (size_t i = 0; i < value_size; ++i) {
        value |= static_cast<T>(data[i]) << (i * 8);
    }

    return value;
}

template <typename T>
void write_uint_le(std::ostream& out, T value) {
    static_assert(std::numeric_limits<T>::is_integer && !std::numeric_limits<T>::is_signed, "write_uint can only write an unsigned integer");
    constexpr const size_t value_size = sizeof(T);

    uint8_t data[value_size];

    for (size_t i = 0; i < value_size; ++i) {
        data[i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
    }

    out.write(reinterpret_cast<char*>(data), value_size);
}

#endif
