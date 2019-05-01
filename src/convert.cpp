#include "convert.h"
#include <string_view>
#include <optional>
#include <fmt/format.h>

namespace {
    enum class FileType {
        Tiff,
        Oct,
        Unknown
    };

    FileType parse_file_type(std::string_view str) {
        if (str == "tiff" || str == "tif") {
            return FileType::Tiff;
        } else if (str == "oct") {
            return FileType::Oct;
        }

        return FileType::Unknown;
    }

    FileType guess_file_type(std::string_view path, const char* arg, std::string_view location_type) {
        std::string_view type_str;

        if (arg) {
            type_str = arg;
        } else {
            const auto extension = path.find_last_of('.');
            if (extension == std::string_view::npos) {
                fmt::print("Error: {} file type not explicitly set and unable to infer from path\n", location_type);
                return FileType::Unknown;
            } else {
                type_str = path.substr(extension + 1);
            }
        }

        const auto type = parse_file_type(type_str);

        if (type == FileType::Unknown) {
            fmt::print("Error: unknown {} file type '{}'\n", location_type, type_str);
        }

        return type;
    }
}

void convert(Span<const char*> args) {
    const char* src = nullptr;
    const char* dst = nullptr;

    const char* src_type_arg = nullptr;
    const char* dst_type_arg = nullptr;

    for (size_t i = 0; i < args.size(); ++i) {
        const auto arg = std::string_view(args[i]);
        const bool is_flag = arg.size() > 0 && arg[0] == '-';

        if (arg == "--src-type" || arg == "-s") {
            if (++i == args.size()) {
                fmt::print("Error: {} expects argument <type>\n", arg);
                return;
            }

            src_type_arg = args[i];
        } else if (arg == "--dst-type" || arg == "-d") {
            if (++i == args.size()) {
                fmt::print("Error: {} expects argument <type>\n", arg);
                return;
            }

            dst_type_arg = args[i];
        } else if (!is_flag && !src) {
            src = args[i];
        } else if (!is_flag && !dst) {
            dst = args[i];
        } else if (is_flag) {
            fmt::print("Error: unrecognized option '{}'\n", arg);
            return;
        } else {
            fmt::print("Error: unrecognized positional argument '{}'\n", arg);
            return;
        }
    }

    if (!src) {
        fmt::print("Error: missing positional argument <source>\n");
        return;
    } else if (!dst) {
        fmt::print("Error: missing positional argument <destination>\n");
        return;
    }

    const auto src_type = guess_file_type(src, src_type_arg, "source");
    const auto dst_type = guess_file_type(dst, dst_type_arg, "destination");

    if (src_type == FileType::Unknown || dst_type == FileType::Unknown) {
        return;
    }

    fmt::print("Wip\n");
}
