#include "convert.h"
#include <vector>
#include <string_view>
#include <optional>
#include <fmt/format.h>
#include "core/arg_parse.h"
#include "model/Grid.h"
#include "model/Octree.h"

namespace {
    enum class FileType {
        Tiff,
        Svo,
        Unknown
    };

    struct ConvertOptions {
        std::string_view src, dst;
        FileType src_type, dst_type;
        long long split_difference;
        bool dag;
    };

    std::string_view to_string(FileType type) {
        switch (type) {
            case FileType::Tiff:
                return "tiff";
            case FileType::Svo:
                return "svo";
            case FileType::Unknown:
            default:
                return "unknown";
        }
    }

    FileType parse_file_type(std::string_view str) {
        if (str == "tiff" || str == "tif") {
            return FileType::Tiff;
        } else if (str == "svo") {
            return FileType::Svo;
        }

        return FileType::Unknown;
    }

    FileType guess_file_type(std::string_view path, const char* override, std::string_view location_type) {
        std::string_view type_str;

        if (override) {
            type_str = override;
        } else {
            const auto extension = path.find_last_of('.');
            if (extension == std::string_view::npos) {
                throw args::ParseError("{} file type not explicitly set and unable to infer from path", location_type);
                return FileType::Unknown;
            } else {
                type_str = path.substr(extension + 1);
            }
        }

        const auto type = parse_file_type(type_str);

        if (type == FileType::Unknown) {
            throw args::ParseError("unknown {} file type '{}'", location_type, type_str);
        }

        return type;
    }

    void convert_tiff_svo(const ConvertOptions& opts) {
        // const auto grid = Grid::load_tiff(opts.src);
        // const auto octree = Octree(grid, static_cast<uint8_t>(opts.split_difference), opts.dag);
    }

    void convert_files(const ConvertOptions& opts) {
        if (opts.src_type == FileType::Tiff && opts.dst_type == FileType::Svo) {
            convert_tiff_svo(opts);
        } else {
            fmt::print("Error: Unsupported conversion of '{}' to '{}'\n", to_string(opts.src_type), to_string(opts.dst_type));
        }
    }
}

void convert(Span<const char*> args) {
    const char* src = nullptr;
    const char* dst = nullptr;

    const char* src_type_override = nullptr;
    const char* dst_type_override = nullptr;

    ConvertOptions opts = {
        .src = "",
        .dst = "",
        .src_type = FileType::Unknown,
        .dst_type = FileType::Unknown,
        .split_difference = 0,
        .dag = false
    };

    auto cmd = args::Command {
        .flags = {
            {&opts.dag, "--dag"}
        },
        .parameters = {
            {args::string_opt(&src_type_override), "source type", "--src-type", 's'},
            {args::string_opt(&dst_type_override), "destination type", "--dst-type", 'd'},
            {args::int_range_opt(&opts.split_difference, 0, 255), "split difference", "--split-difference"}
        },
        .positional = {
            {args::string_opt(&src), "source file"},
            {args::string_opt(&dst), "destination file"}
        }
    };

    try {
        args::parse(args, cmd);
        opts.src_type = guess_file_type(src, src_type_override, "source");
        opts.dst_type = guess_file_type(dst, dst_type_override, "destination");
    } catch (const args::ParseError& e) {
        fmt::print("Error: {}\n", e.what());
        return;
    }

    opts.src = src;
    opts.dst = dst;

    convert_files(opts);
}
