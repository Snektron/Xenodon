#ifndef _XENODON_CORE_ARG_PARSE_H
#define _XENODON_CORE_ARG_PARSE_H

#include <string_view>
#include <vector>
#include <tuple>
#include <array>
#include <cstdint>
#include "utility/Span.h"

namespace args {
    struct Parameter {
        const char** variable;
        bool seen;
        std::string_view value_name;
        std::string_view long_version;
        int short_version;

        Parameter(const char*& variable, std::string_view value_name, std::string_view long_version);
        Parameter(const char*& variable, std::string_view value_name, std::string_view long_version, char short_version);
    };

    struct Flag {
        bool* variable;
        bool seen;
        std::string_view long_version;
        int short_version;

        Flag(bool& variable, std::string_view long_version);
        Flag(bool& variable, std::string_view long_version, char short_version);
    };

    struct Positional {
        const char** variable;
        std::string_view name;

        Positional(const char*& variable, std::string_view name);
    };

    struct Command {
        std::vector<Flag> flags = {};
        std::vector<Parameter> parameters = {};
        std::vector<Positional> positional = {};
    };

    bool parse(Span<const char*> args, Command& cmd);
}

namespace arg {
    struct Value {
        virtual ~Value() = default;
        virtual bool parse(std::string_view arg) = 0;
    };

    struct StringValue: public Value {
        std::string value;

    public:
        StringValue(std::string_view default_value = ""):
            value(default_value) {
        }

        bool parse(std::string_view arg) override;

        inline std::string_view get() const {
            return value;
        }
    };

    struct NumberValue: public Value {
        int64_t min;
        int64_t max;
        int64_t value;

    public:
        NumberValue(int64_t min, int64_t max, int64_t default_value = 0):
            min(min),
            max(max),
            value(default_value) {
        }

        bool parse(std::string_view arg) override;

        inline int64_t get() const {
            return value;
        }
    };

    struct Flag {
        bool* variable;
        std::string long_arg;
        int short_arg;
        bool seen;

        Flag(bool* variable, std::string_view long_arg, int short_arg = 0):
            variable(variable),
            long_arg(long_arg),
            short_arg(short_arg),
            seen(false) {
        }
    };

    struct Parameter {
        Value* variable;
        std::string value_name;
        std::string long_arg;
        int short_arg;
        bool seen;

        Parameter(Value* variable, std::string_view value_name, std::string_view long_arg, int short_arg = 0):
            variable(variable),
            value_name(value_name),
            long_arg(long_arg),
            short_arg(short_arg),
            seen(false) {
        }
    };

    struct Positional {
        Value* variable;
        std::string value_name;
    };

    struct Command {
        std::vector<Flag> flags = {};
        std::vector<Parameter> parameters = {};
        std::vector<Positional> positional = {};
    };

    bool parse(Span<const char*> args, Command& cmd);
}

#endif
