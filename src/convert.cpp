#include "convert.h"
#include <vector>
#include <string_view>
#include <optional>
#include <fmt/format.h>
#include "core/arg_parse.h"

namespace {
    enum class FileType {
        Tiff,
        Oct,
        Unknown
    };

    struct ConvertOptions {
        uint8_t split_difference;
        bool dag;
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

    ConvertOptions opts;

    auto cmd = args::Command {
        .flags = {
            {opts.dag, "--dag"}
        },
        .parameters = {
            {src_type_arg, "source type", "--src-type", 's'},
            {dst_type_arg, "destination type", "--dst-type", 'd'}
        },
        .positional = {
            {src, "source file"},
            {dst, "destination file"}
        }
    };

    if (!args::parse(args, cmd)) {
        return;
    }

    const auto src_type = guess_file_type(src, src_type_arg, "source file");
    const auto dst_type = guess_file_type(dst, dst_type_arg, "destination file");

    if (src_type == FileType::Unknown || dst_type == FileType::Unknown) {
        return;
    }

    fmt::print("Wip\n");
}
