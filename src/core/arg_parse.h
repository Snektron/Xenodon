#ifndef _XENODON_CORE_ARG_PARSE_H
#define _XENODON_CORE_ARG_PARSE_H

#include <string_view>
#include <vector>
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

#endif
