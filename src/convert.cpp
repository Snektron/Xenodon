#include "convert.h"
#include <string_view>
#include <filesystem>
#include <stdexcept>
#include <memory>
#include <fmt/format.h>
#include "core/arg_parse.h"
#include "core/Error.h"
#include "model/Grid.h"
#include "model/Octree.h"

void convert(Span<const char*> args) {
    auto src = std::filesystem::path();
    auto dst = std::filesystem::path();

    uint8_t split_difference = 0;
    bool dag = false;

    auto cmd = args::Command {
        .flags = {
            {&dag, "--dag"}
        },
        .parameters = {
            {args::int_range_opt<uint8_t>(&split_difference), "split difference", "--split-difference"}
        },
        .positional = {
            {args::path_opt(&src), "source tiff path"},
            {args::path_opt(&dst), "destination svo path"}
        }
    };

    try {
        args::parse(args, cmd);
    } catch (const args::ParseError& e) {
        fmt::print("Error: {}\n", e.what());
        return;
    }

    std::unique_ptr<Grid> grid;

    try {
        grid = std::make_unique<Grid>(Grid::load_tiff(src));
    } catch (const Error& e) {
        fmt::print("Error reading '{}': {}", src.native(), e.what());
        return;
    }

    {
        auto dim = grid->dimensions();
        fmt::print("Source grid:\n");
        fmt::print(" Dimensions: {}x{}x{}\n", dim.x, dim.y, dim.z);
        fmt::print(" Size: {:n} bytes\n", grid->memory_footprint());
    }

    const auto octree = Octree(*grid, split_difference, dag);

    {
        fmt::print("Generated octree:\n");
        fmt::print(" Dimensions: {0}x{0}x{0}\n", octree.side());
        fmt::print(" Size: {:n} bytes\n", octree.memory_footprint());
        fmt::print(" Nodes: {:n}\n", octree.data().size());
    }

    try {
        octree.save_svo(dst);
    } catch (const std::runtime_error& e) {
        fmt::print("Error writing '{}': {}", dst.native(), e.what());
    }
}
