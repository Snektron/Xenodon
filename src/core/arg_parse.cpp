#include "core/arg_parse.h"
#include <fmt/format.h>
#include "core/Parser.h"

namespace args {
    Parameter::Parameter(const char*& variable, std::string_view value_name, std::string_view long_version):
        variable(&variable),
        seen(false),
        value_name(value_name),
        long_version(long_version),
        short_version(-1) {
    }

    Parameter::Parameter(const char*& variable, std::string_view value_name, std::string_view long_version, char short_version):
        variable(&variable),
        seen(false),
        value_name(value_name),
        long_version(long_version),
        short_version(short_version) {
    }

    Flag::Flag(bool& variable, std::string_view long_version):
        variable(&variable),
        seen(false),
        long_version(long_version),
        short_version(-1) {
    }

    Flag::Flag(bool& variable, std::string_view long_version, char short_version):
        variable(&variable),
        seen(false),
        long_version(long_version),
        short_version(short_version) {
    }

    Positional::Positional(const char*& variable, std::string_view name):
        variable(&variable), name(name) {
    }

    bool parse(Span<const char*> args, Command& cmd) {
        auto flag_or_option = [](std::string_view arg) {
            return arg.size() > 0 && arg[0] == '-';
        };

        auto find_flag = [&cmd](std::string_view arg) -> Flag* {
            for (auto& flag : cmd.flags) {
                if (arg == flag.long_version || (arg.size() == 2 && arg[1] == flag.short_version)) {
                    return &flag;
                }
            }

            return nullptr;
        };

        auto find_parameter = [&cmd](std::string_view arg) -> Parameter* {
            for (auto& param : cmd.parameters) {
                if (arg == param.long_version || (arg.size() == 2 && arg[1] == param.short_version)) {
                    return &param;
                }
            }

            return nullptr;
        };

        size_t pos_args_seen = 0;

        for (size_t i = 0; i < args.size(); ++i) {
            const auto arg = std::string_view(args[i]);
            if (flag_or_option(arg)) {
                if (Flag* flag = find_flag(arg)) {
                    if (flag->seen) {
                        fmt::print("Error: Duplicate specification of flag {}/{}\n", flag->long_version, flag->short_version);
                        return false;
                    }

                    flag->seen = true;
                    *flag->variable = true;
                } else if (Parameter* param = find_parameter(arg)) {
                    ++i;
                    if (param->seen) {
                        fmt::print("Error: Duplicate specification of parameter {}/-{}\n", param->long_version, (char) param->short_version);
                        return false;
                    } else if (i == args.size() || flag_or_option(args[i])) {
                        fmt::print("Error: Parameter {} expects argument <{}>\n", arg, param->value_name);
                        return false;
                    }

                    param->seen = true;
                    *param->variable = args[i];
                } else {
                    fmt::print("Error: Unrecognized option {}\n", arg);
                    return false;
                }
            } else if (pos_args_seen == cmd.positional.size()) {
                fmt::print("Error: Unexpected positional argument '{}'\n", arg);
                return false;
            } else {
                *cmd.positional[pos_args_seen].variable = args[i];
                ++pos_args_seen;
            }
        }

        if (pos_args_seen != cmd.positional.size()) {
            for (size_t i = pos_args_seen; i < cmd.positional.size(); ++i) {
                fmt::print("Error: Missing required positional argument <{}>\n", cmd.positional[i].name);
            }

            return false;
        }

        return true;
    }
}

namespace arg {
    bool StringValue::parse(std::string_view arg) {
        this->value = arg;
        return true;
    }

    bool NumberValue::parse(std::string_view arg) {
        bool negative = false;
        if (arg.size() > 0 && arg[0] == '-') {
            negative = true;
            arg.remove_prefix(1);
        }

        size_t parsed_value;
        try {
            parsed_value = parse<size_t>(arg);
        } catch (parser::ParseError& e) {
            return false;
        }

        int64_t value = static_cast<int64_t>(parsed_value) * (negative ? -1 : 1);
    }

    bool parse(Span<const char*> args, Command& cmd) {

    }
}
