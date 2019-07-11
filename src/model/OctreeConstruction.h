#ifndef _XENODON_MODEL_OCTREECONSTRUCTION_H
#define _XENODON_MODEL_OCTREECONSTRUCTION_H

#include <vector>
#include <unordered_map>
#include <utility>
#include <cstdint>
#include <cstddef>
#include <fmt/format.h>
#include "model/Octree.h"
#include "model/Grid.h"

struct NoopCache {
    uint32_t operator()([[maybe_unused]] const Octree::Node& node, uint32_t index) {
        return index;
    }
};

struct HashCache {
    std::unordered_map<Octree::Node, uint32_t> cache;

    uint32_t operator()(const Octree::Node& node, uint32_t index) {
        const auto [it, inserted] = this->cache.insert({node, index});
        if (inserted) {
            return index;
        }

        return it->second;
    }
};

struct ChannelDiffHeuristic {
    uint8_t channel_diff;

    std::pair<Pixel, bool> grid_scan(const Grid& grid, const Vec3Sz& offset, size_t extent) const {
        const auto [avg, max_diff] = grid.vol_scan(offset, offset + extent);
        return {avg, max_diff > this->channel_diff};
    }
};

struct StdDevHeuristic {
    double stddev;

    std::pair<Pixel, bool> grid_scan(const Grid& grid, const Vec3Sz& offset, size_t extent) const {
        const auto [avg, stddev] = grid.stddev_scan(offset, offset + extent);
        return {avg, stddev > this->stddev};
    }
};

struct ConstructionStats {
    size_t total_leaves;
    size_t unique_leaves;
    size_t total_nodes;
    size_t depth;

    ConstructionStats():
        total_leaves(0),
        unique_leaves(0),
        total_nodes(0),
        depth(0) {
    }
};

namespace detail {
    template <typename Cache>
    struct OctreeBuilder {
        size_t dim;
        std::vector<Octree::Node> nodes;
        Cache cache;

        OctreeBuilder(size_t dim, const Cache& cache):
            dim(dim), cache(cache) {
        }

        std::pair<uint32_t, bool> insert(const Octree::Node& node) {
            const uint32_t end_index = static_cast<uint32_t>(this->nodes.size());
            const uint32_t actual_index = this->cache(node, end_index);
            const bool inserted = actual_index == end_index;

            if (inserted) {
                // The node was not present in the cache, so insert it at the end of the list
                this->nodes.push_back(node);

                if (this->nodes.size() % 1'000'000 == 0) {
                    fmt::print("{} nodes...\n", this->nodes.size());
                }
            }

            return {actual_index, inserted};
        }

        Octree build() && {
            this->nodes.shrink_to_fit();
            std::reverse(this->nodes.begin(), this->nodes.end());

            const uint32_t end = static_cast<uint32_t>(this->nodes.size()) - 1;

            for (auto& node : this->nodes) {
                // If the node is a leaf, reset all of its child pointers to the root.
                if (node.is_leaf()) {
                    for (uint32_t& child : node.children) {
                        child = Octree::ROOT;
                    }
                } else {
                    for (uint32_t& child : node.children) {
                        child = end - child;
                    }
                }
            }

            return Octree(this->dim, std::move(this->nodes));
        }
    };

    template <typename SplitHeuristic, typename Cache>
    struct Context {
        const Grid& grid;
        const SplitHeuristic& heuristic;
        OctreeBuilder<Cache> builder;
        ConstructionStats& stats;
    };

    template <typename SplitHeuristic, typename Cache>
    uint32_t construct(Context<SplitHeuristic, Cache>& ctx, const Vec3Sz& offset, size_t extent, size_t depth) {
        ctx.stats.depth = std::max(ctx.stats.depth, depth);

        auto insert = [&ctx](const Octree::Node& node, bool leaf) {
            auto [index, inserted] = ctx.builder.insert(node);

            ++ctx.stats.total_nodes;
            if (leaf) {
                ++ctx.stats.total_leaves;
                if (inserted) {
                    ++ctx.stats.unique_leaves;
                }
            }

            return index;
        };

        // Check if the current area [offset, offset + extent) is totally outside the source grid
        const bool totally_in_grid = offset.x < ctx.grid.dimensions().x &&
            offset.y < ctx.grid.dimensions().y &&
            offset.z < ctx.grid.dimensions().z;

        if (!totally_in_grid) {
            const auto node = Octree::Node{
                .children = {0},
                .color = Pixel{0, 0, 0, 0},
                .is_leaf_depth = Octree::LEAF | static_cast<uint32_t>(depth),
            };

            return insert(node, true);
        }

        // Check if the current area [offset, offset + extent] is partly outside the source grid
        const bool partly_in_grid = offset.x + extent < ctx.grid.dimensions().x &&
            offset.y + extent < ctx.grid.dimensions().y &&
            offset.z + extent < ctx.grid.dimensions().z;

        const auto [avg, split] = ctx.heuristic.grid_scan(ctx.grid, offset, extent);

        if ((!split && partly_in_grid) || extent == 1) {
            // This node is a leaf node
            const auto node = Octree::Node{
                .children = {0},
                .color = avg,
                .is_leaf_depth = Octree::LEAF | static_cast<uint32_t>(depth),
            };

            return insert(node, true);
        } else {
            // This node is an intermediary node
            const size_t h_extent = extent / 2;
            size_t child = 0;

            auto node = Octree::Node{
                .children = {},
                .color = avg,
                .is_leaf_depth = static_cast<uint32_t>(depth),
            };

            for (auto xoff : {size_t{0}, h_extent}) {
                for (auto yoff : {size_t{0}, h_extent}) {
                    for (auto zoff : {size_t{0}, h_extent}) {
                        uint32_t index = construct(ctx, {offset.x + xoff, offset.y + yoff, offset.z + zoff}, h_extent, depth + 1);
                        node.children[child++] = index;
                    }
                }
            }

            return insert(node, false);
        }
    }

    template <typename SplitHeuristic, typename Cache>
    Octree build_octree(const Grid& grid, ConstructionStats& stats, const SplitHeuristic& heuristic, const Cache& cache) {
        // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
        const auto ceil_2pow = [](uint64_t x) {
            --x;
            x |= x >> 1;
            x |= x >> 2;
            x |= x >> 4;
            x |= x >> 8;
            x |= x >> 16;
            x |= x >> 32;
            return ++x;
        };

        const auto src_dim = grid.dimensions();
        const auto dim = std::max({ceil_2pow(src_dim.x), ceil_2pow(src_dim.y), ceil_2pow(src_dim.z)});

        auto context = detail::Context<SplitHeuristic, Cache> {
            grid,
            heuristic,
            detail::OctreeBuilder(dim, cache),
            stats
        };

        detail::construct(context, Vec3Sz(0), dim, 0);

        return std::move(context.builder).build();
    }
}

template <typename SplitHeuristic>
Octree build_octree(const Grid& grid, ConstructionStats& stats, const SplitHeuristic& heuristic, Octree::Type type) {
    auto octree = type == Octree::Type::Dag ?
        detail::build_octree(grid, stats, heuristic, HashCache{}) :
        detail::build_octree(grid, stats, heuristic, NoopCache{});

    if (type == Octree::Type::Rope) {
        octree.generate_ropes();
    }

    return std::move(octree);
}

#endif
