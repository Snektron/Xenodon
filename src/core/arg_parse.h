#ifndef _XENODON_CORE_ARG_PARSE_H
#define _XENODON_CORE_ARG_PARSE_H

#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <type_traits>
#include <cstdint>
#include "utility/Span.h"
#include "core/Error.h"

namespace args {
    using Action = std::function<bool(const char*)>;

    struct Parameter {
        Action action;
        std::string value_name;
        std::string long_arg;
        int short_arg;
        bool seen;

        Parameter(const Action& action, std::string_view value_name, std::string_view long_arg, int short_arg = -1):
            action(action),
            value_name(value_name),
            long_arg(long_arg),
            short_arg(short_arg),
            seen(false) {
        }
    };

    struct Flag {
        bool* variable;
        std::string long_arg;
        int short_arg;
        bool seen;

        Flag(bool* variable, std::string_view long_arg, int short_arg = -1):
            variable(variable),
            long_arg(long_arg),
            short_arg(short_arg),
            seen(false) {
        }
    };

    struct Positional {
        Action action;
        std::string name;

        Positional(const Action& action, std::string_view name):
            action(action),
            name(name) {
        }
    };

    struct Command {
        std::vector<Flag> flags = {};
        std::vector<Parameter> parameters = {};
        std::vector<Positional> positional = {};
    };

    struct ParseError: public Error {
        template <typename... Args>
        ParseError(const Args&... args):
            Error(args...) {
        }
    };

    void parse(Span<const char*> args, Command& cmd);

    long long parse_int(std::string_view, long long min, long long max);

    inline constexpr auto string_opt(const char** var) {
        return [var](const char* arg) {
            *var = arg;
            return true;
        };
    }

    template <typename T, typename U>
    constexpr const bool is_integer_subset =
           std::numeric_limits<T>::is_integer
        && std::numeric_limits<U>::is_integer
        && std::numeric_limits<T>::min() >= std::numeric_limits<U>::min()
        && std::numeric_limits<T>::max() <= std::numeric_limits<U>::max();

    template <typename T, typename = std::enable_if_t<is_integer_subset<T, long long>>>
    inline constexpr auto int_range_opt(T* var, long long min, long long max) {
        return [var, min, max](const char* arg) {
            try {
                *var = static_cast<T>(parse_int(arg, min, max));
                return true;
            } catch (const ParseError&) {
                return false;
            }
        };
    }
}

#endif
