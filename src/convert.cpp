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
#include "model/OctreeConstruction.h"

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

    auto stats = ConstructionStats();
    auto convert_octree = [&](auto heuristic) {
        const auto type = dag ? Octree::Type::Dag : rope ? Octree::Type::Rope : Octree::Type::Sparse;
        return build_octree(*grid, stats, heuristic, type);
    };

    auto octree = stddev >= 0 ?
        convert_octree(StdDevHeuristic{stddev}) :
        convert_octree(ChannelDiffHeuristic{
                static_cast<uint8_t>(std::max(channel_difference, 0))
        });

    {
        auto k_ary_nodes = [](size_t k, size_t h) {
            auto ipow = [](size_t x, size_t y) {
                size_t z = 1;

                while (--y) {
                    z *= x;
                }

                return z;
            };

            return (ipow(k, h + 1) - 1) / (k + 1);
        };

        size_t perfect_tree_nodes = k_ary_nodes(8, stats.depth);
        double total_nodes_proportion = static_cast<double>(stats.total_nodes) / static_cast<double>(perfect_tree_nodes);
        double unique_nodes_proportion = static_cast<double>(octree.data().size()) / static_cast<double>(perfect_tree_nodes);

        fmt::print("Generated octree:\n");
        fmt::print(" Dimensions: {0}x{0}x{0}\n", octree.side());
        fmt::print(" Size: {:n} bytes\n", octree.memory_footprint());
        fmt::print(" Perfect tree nodes: {:n}\n", perfect_tree_nodes);
        fmt::print(" Total nodes: {:n} ({:.5f}%)\n", stats.total_nodes, total_nodes_proportion * 100);
        fmt::print(" Unique nodes: {:n} ({:.5f}%)\n", octree.data().size(), unique_nodes_proportion * 100);
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
