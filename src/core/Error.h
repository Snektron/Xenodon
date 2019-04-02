#ifndef _XENODON_CORE_ERROR_H
#define _XENODON_CORE_ERROR_H

#include <stdexcept>
#include <string_view>
#include <fmt/format.h>

struct Error: public std::runtime_error {
    template <typename... Args>
    Error(std::string_view fmt, const Args&... args):
        runtime_error(fmt::format(fmt, args...)) {
    }
};

#endif
