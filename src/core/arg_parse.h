#ifndef _XENODON_CORE_ARG_PARSE_H
#define _XENODON_CORE_ARG_PARSE_H

#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <type_traits>
#include <charconv>
#include <system_error>
#include <filesystem>
#include <cctype>
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

    template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    bool parse_float(std::string_view str, T& out) {
        for (auto c : str) {
            if (!std::isdigit(c) && c != '.' && c != '-') {
                return false;
            }
        }

        T value;
        char* end;
        auto cpp_cant_into_float_parsing = std::string(str);

        if constexpr (std::is_same_v<T, float>) {
            value = std::strtof(cpp_cant_into_float_parsing.c_str(), &end);
        } else if constexpr (std::is_same_v<T, double>) {
            value = std::strtod(cpp_cant_into_float_parsing.c_str(), &end);
        } else if constexpr (std::is_same_v<T, long double>) {
            value = std::strtold(cpp_cant_into_float_parsing.c_str(), &end);
        }

        if (end != &*cpp_cant_into_float_parsing.end()) {
            return false;
        }

        out = value;
        return true;
    }

    constexpr auto string_opt(const char** var) {
        return [var](const char* arg) {
            *var = arg;
            return true;
        };
    }

    constexpr auto string_opt(std::string_view* var) {
        return [var](std::string_view arg) {
            *var = arg;
            return true;
        };
    }

    template <typename T, typename = std::enable_if_t<std::numeric_limits<T>::is_integer>>
    constexpr auto int_range_opt(T* var, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max()) {
        return [var, min, max](std::string_view arg) {
            T value;
            auto [end, err] = std::from_chars(arg.begin(), arg.end(), value);
            if (err != std::errc() || end != arg.end()) {
                return false;
            } else if (value < min || value > max) {
                return false;
            }

            *var = value;
            return true;
        };
    }

    template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    constexpr auto float_range_opt(T* var, T min = std::numeric_limits<T>::lowest(), T max = std::numeric_limits<T>::max()) {
        return [var, min, max](std::string_view arg) {
            T value;
            if (!parse_float(arg, value)) {
                return false;
            }

            if (value < min || value > max) {
                return false;
            }

            *var = value;
            return true;
        };
    }

    constexpr auto path_opt(std::filesystem::path* var) {
        return [var](std::string_view arg) {
            *var = arg;
            return true;
        };
    }
}

#endif
