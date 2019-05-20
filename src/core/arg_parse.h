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
        return [var, min, max](const char* arg) {
            for (auto c : std::string_view(arg)) {
                if (!std::isdigit(c) && c != '.' && c != '-') {
                    return false;
                }
            }

            T value;
            char* end;

            if constexpr (std::is_same_v<T, float>) {
                value = std::strtof(arg, &end);
            } else if constexpr (std::is_same_v<T, double>) {
                value = std::strtod(arg, &end);
            } else if constexpr (std::is_same_v<T, long double>) {
                value = std::strtold(arg, &end);
            }

            if (end == arg) {
                return false;
            } else if (value < min || value > max) {
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