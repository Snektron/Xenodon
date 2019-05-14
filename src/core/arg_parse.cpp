#include "core/arg_parse.h"
#include <fmt/format.h>
#include "core/Parser.h"

namespace args {
    void parse(Span<const char*> args, Command& cmd) {
        auto flag_or_option = [](std::string_view arg) {
            return arg.size() > 0 && arg[0] == '-';
        };

        auto find_flag = [&cmd](std::string_view arg) -> Flag* {
            for (auto& flag : cmd.flags) {
                if (arg == flag.long_arg || (arg.size() == 2 && arg[1] == flag.short_arg)) {
                    return &flag;
                }
            }

            return nullptr;
        };

        auto find_parameter = [&cmd](std::string_view arg) -> Parameter* {
            for (auto& param : cmd.parameters) {
                if (arg == param.long_arg || (arg.size() == 2 && arg[1] == param.short_arg)) {
                    return &param;
                }
            }

            return nullptr;
        };

        auto duplicate_err = [](std::string_view type, std::string_view long_arg, int short_arg) {
            if (short_arg != -1) {
                return ParseError("Duplicate specification of {} {}/-{}", type, long_arg, static_cast<char>(short_arg));
            } else {
                return ParseError("Duplicate specification of {} {}", type, long_arg);
            }
        };

        size_t pos_args_seen = 0;

        for (size_t i = 0; i < args.size(); ++i) {
            const auto arg = std::string_view(args[i]);
            if (flag_or_option(arg)) {
                if (Flag* flag = find_flag(arg)) {
                    if (flag->seen) {
                        throw duplicate_err("flag", flag->long_arg, flag->short_arg);
                    }

                    flag->seen = true;
                    *flag->variable = true;
                } else if (Parameter* param = find_parameter(arg)) {
                    ++i;
                    if (param->seen) {
                        throw duplicate_err("parameter", param->long_arg, param->short_arg);
                    } else if (i == args.size()) {
                        throw ParseError("Parameter {} expects argument <{}>", arg, param->value_name);
                    }

                    param->seen = true;
                    bool parsed = param->action(args[i]);
                    if (!parsed) {
                        throw ParseError("Invalid value for <{}> of parameter {}", param->value_name, arg);
                    }
                } else {
                    throw ParseError("Unrecognized option {}", arg);
                }
            } else if (pos_args_seen == cmd.positional.size()) {
                throw ParseError("Unexpected positional argument '{}'", arg);
            } else {
                auto& positional = cmd.positional[pos_args_seen];
                bool parsed = positional.action(args[i]);
                if (!parsed) {
                    throw ParseError("Invalid value for positional argument <{}>", positional.name);
                }

                ++pos_args_seen;
            }
        }

        if (pos_args_seen != cmd.positional.size()) {
            throw ParseError("Missing required positional argument <{}>", cmd.positional[pos_args_seen].name);
        }
    }

    long long parse_int(std::string_view str, long long min, long long max) {
        long long val;
        size_t processed;

        try {
            val = std::stoll(std::string(str), &processed);
        } catch (const std::invalid_argument&) {
            throw ParseError("Failed to convert to int");
        } catch (const std::out_of_range&) {
            throw ParseError("Out of range");
        }

        if (processed != str.size()) {
            throw ParseError("Failed to convert to int");
        }

        if (val < min || val > max) {
            throw ParseError("Out of range");
        }

        return val;
    }
}
