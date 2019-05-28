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

    // uint8_t split_difference = 0;
    bool dag = false;
    bool rope = false;

    int channel_difference = -1;
    double stddev = -1;

    auto cmd = args::Command {
        .flags = {
            {&dag, "--dag"},
            {&rope, "--rope"}
        },
        .parameters = {
            {args::int_range_opt<int>(&channel_difference, 0, 255), "channel difference", "--chan-diff"},
            {args::float_range_opt(&stddev, 0.0), "std. dev", "--std-dev"}
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

    if (dag && rope) {
        fmt::print("Error: --dag and --rope are mutually exclusive\n");
        return;
    }

    if (channel_difference >= 0 && stddev >= 0) {
        fmt::print("Error: --std-dev and --chan-diff are mutually exclusive\n");
        return;
    }

    fmt::print("Loading source...\n");
    std::unique_ptr<Grid> grid;

    try {
        grid = std::make_unique<Grid>(Grid::load_tiff(src));
    } catch (const Error& e) {
        fmt::print("Error reading '{}': {}\n", src.native(), e.what());
        return;
    }

    {
        auto dim = grid->dimensions();
        fmt::print("Source grid:\n");
        fmt::print(" Dimensions: {}x{}x{}\n", dim.x, dim.y, dim.z);
        fmt::print(" Size: {:n} bytes\n", grid->memory_footprint());
    }

    fmt::print("Converting to octree...\n");

    Octree::SplitHeuristic heuristic = Octree::ChannelDiffHeuristic{0};
    if (channel_difference > 0) {
        heuristic = Octree::ChannelDiffHeuristic{static_cast<uint8_t>(channel_difference)};
    } else if (stddev >= 0) {
        heuristic = Octree::StdDevHeuristic{stddev};
    }

    const auto [octree, stats] = Octree::from_grid({
        .src = *grid,
        .type = dag ? Octree::Type::Dag : rope ? Octree::Type::Rope : Octree::Type::Sparse,
        .heuristic = heuristic,
        .report = [](size_t nodes) {
            fmt::print("{:n} nodes...\n", nodes);
        }
    });

    {
        fmt::print("Generated octree:\n");
        fmt::print(" Dimensions: {0}x{0}x{0}\n", octree.side());
        fmt::print(" Size: {:n} bytes\n", octree.memory_footprint());
        fmt::print(" Unique nodes: {:n}\n", octree.data().size());
        fmt::print(" Total nodes: {:n}\n", stats.total_nodes);
        fmt::print(" Total leaves: {:n}\n", stats.total_leaves);
        fmt::print(" Unique leaves: {:n}\n", stats.unique_leaves);
        fmt::print(" Depth: {:n}\n", stats.depth);
    }

    try {
        octree.save_svo(dst);
    } catch (const std::runtime_error& e) {
        fmt::print("Error writing '{}': {}\n", dst.native(), e.what());
    }
}
