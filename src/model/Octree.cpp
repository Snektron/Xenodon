#include "model/Octree.h"
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include <string_view>
#include <cmath>
#include <fmt/format.h>
#include "core/Logger.h"
#include "core/Error.h"
#include "model/Grid.h"
#include "utility/serialization.h"

namespace {
    // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    uint64_t ceil_2pow(uint64_t x) {
        --x;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        x |= x >> 32;
        return ++x;
    }

    // Taken from boost:
    // https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
    template <typename T>
    size_t hash_combine(size_t seed, const T& v) {
        return seed ^ (std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }

    constexpr const std::string_view SVO_FMT_ID = "XNDN-SVO";
}

template<>
struct std::hash<Octree::Node> {
    size_t operator()(const Octree::Node& node) const {
        size_t v = std::hash<uint32_t>{}(node.is_leaf_depth);
        v = hash_combine(v, std::hash<uint32_t>{}(node.color.pack()));

        for (uint32_t child : node.children) {
            v = hash_combine(v, std::hash<uint32_t>{}(child));
        }

        return v;
    }
};

bool operator==(const Octree::Node& lhs, const Octree::Node& rhs) {
    if (lhs.is_leaf_depth != rhs.is_leaf_depth || lhs.color != rhs.color)
        return false;

    return std::equal(std::begin(lhs.children), std::end(lhs.children), std::begin(rhs.children));
}

bool operator!=(const Octree::Node& lhs, const Octree::Node& rhs) {
    return !(lhs == rhs);
}

struct Octree::ConstructionContext {
    std::unordered_map<Node, uint32_t> map = {};
    const Octree::ConstructionParameters& params;
    Octree::ConstructionStats stats;
};

Octree::Octree(size_t dim, std::vector<Node>&& nodes):
    dim(dim), nodes(std::move(nodes)) {
}

std::pair<Octree, Octree::ConstructionStats> Octree::from_grid(const ConstructionParameters& params) {
    auto ctx = ConstructionContext{
        .params = params,
        .stats = {
            .total_leaves = 0,
            .unique_leaves = 0,
            .total_nodes = 0,
            .depth = 0
        }
    };

    const auto src_dim = params.src.dimensions();
    auto dim = std::max({ceil_2pow(src_dim.x), ceil_2pow(src_dim.y), ceil_2pow(src_dim.z)});

    auto octree = Octree(dim, std::vector<Octree::Node>());

    octree.construct(ctx, Vec3Sz{0, 0, 0}, dim, 0);
    octree.nodes.shrink_to_fit();

    std::reverse(octree.nodes.begin(), octree.nodes.end());

    uint32_t end = static_cast<uint32_t>(octree.nodes.size()) - 1;

    for (auto& node : octree.nodes) {
        // If the node is a leaf, reset all of its child pointers to the root.
        if (node.is_leaf()) {
            for (uint32_t& child : node.children) {
                child = ROOT;
            }
        } else {
            for (uint32_t& child : node.children) {
                child = end - child;
            }
        }
    }

    if (params.type == Type::Rope) {
        octree.generate_ropes();
    }

    return {std::move(octree), std::move(ctx.stats)};
}

Octree Octree::load_svo(const std::filesystem::path& path) {
    auto in = std::ifstream(path, std::ios::binary);
    if (!in) {
        throw Error("Failed to open");
    }

    char id[SVO_FMT_ID.size()];
    in.read(id, SVO_FMT_ID.size());
    if (SVO_FMT_ID != std::string_view(id, SVO_FMT_ID.size())) {
        fmt::print("Fmt id: '{}', got: '{}'\n", SVO_FMT_ID, std::string_view(id, SVO_FMT_ID.size()));
        throw Error("Invalid format id");
    }

    uint64_t dim = read_uint_le<uint64_t>(in);
    uint64_t num_nodes = read_uint_le<uint64_t>(in);

    auto pos = in.tellg();
    if (pos == std::ifstream::pos_type(-1)) {
        throw Error("Failed to tell");
    }

    in.seekg(0, std::ios_base::end);
    auto end = in.tellg();
    if (end == std::ifstream::pos_type(-1)) {
        throw Error("Failed to tell");
    }

    in.seekg(pos);
    size_t remaining = static_cast<size_t>(end - pos);
    if (remaining != sizeof(Node) * num_nodes) {
        throw Error("File size does not match number of nodes");
    }

    auto nodes = std::vector<Node>(num_nodes);
    for (auto& node : nodes) {
        for (uint32_t& child : node.children) {
            child = read_uint_le<uint32_t>(in);
        }

        node.color = Pixel::unpack(read_uint_le<uint32_t>(in));
        node.is_leaf_depth = read_uint_le<uint32_t>(in);
    }

    return Octree(static_cast<size_t>(dim), std::move(nodes));
}

void Octree::save_svo(const std::filesystem::path& path) const {
    auto out = std::ofstream(path, std::ios::binary);
    if (!out) {
        throw Error("Failed to open");
    }

    out.write(SVO_FMT_ID.data(), SVO_FMT_ID.size());
    write_uint_le(out, this->dim);
    write_uint_le(out, this->nodes.size());

    for (const auto& node : this->nodes) {
        for (uint32_t child : node.children) {
            write_uint_le(out, child);
        }

        write_uint_le(out, node.color.pack());
        write_uint_le(out, node.is_leaf_depth);
    }
}

std::pair<const Octree::Node*, size_t> Octree::find(const Vec3Sz& pos, size_t max_depth) const {
    size_t extent = this->dim;
    if (pos.x >= extent || pos.y >= extent || pos.z >= extent) {
        return {nullptr, 0};
    }

    size_t index = 0;
    auto offset = Vec3Sz{0, 0, 0};

    while (true) {
        extent /= 2;

        if (this->nodes[index].is_leaf() || extent == 0 || max_depth == 0) {
            return {&this->nodes[index], index};
        }

        size_t child_index = 0;

        if (pos.x >= offset.x + extent) {
            child_index |= X_POS;
            offset.x += extent;
        }

        if (pos.y >= offset.y + extent) {
            child_index |= Y_POS;
            offset.y += extent;
        }

        if (pos.z >= offset.z + extent) {
            child_index |= Z_POS;
            offset.z += extent;
        }

        index = this->nodes[index].children[child_index];

        --max_depth;
    }
}

uint32_t Octree::construct(ConstructionContext& ctx, Vec3Sz offset, size_t extent, size_t depth) {
    ctx.stats.depth = std::max(ctx.stats.depth, depth);

    auto push_node = [&](const Node& node) {
        this->nodes.push_back(node);
        if (ctx.params.report && this->nodes.size() % 1'000'000 == 0) {
            ctx.params.report(this->nodes.size());
        }
    };

    auto insert = [&](const Node& node) {
        uint32_t node_index = static_cast<uint32_t>(this->nodes.size());
        bool leaf = node.is_leaf();
        ++ctx.stats.total_nodes;

        if (ctx.params.type == Type::Dag) {
            auto [it, inserted] = ctx.map.insert({node, node_index});
            if (inserted) {
                push_node(node);

                if (leaf) {
                    ++ctx.stats.unique_leaves;
                }
            }

            if (leaf) {
                ++ctx.stats.total_leaves;
            }

            return it->second;
        } else {
            push_node(node);

            if (leaf) {
                ++ctx.stats.total_leaves;
                ++ctx.stats.unique_leaves;
            }

            return node_index;
        }
    };

    bool isect_min = offset.x < ctx.params.src.dimensions().x &&
        offset.y < ctx.params.src.dimensions().y &&
        offset.z < ctx.params.src.dimensions().z;

    // The current area is outside the source area
    if (!isect_min) {
        const auto node = Node{
            .children = {0},
            .color = Pixel{0, 0, 0, 0},
            .is_leaf_depth = LEAF | static_cast<uint32_t>(depth),
        };
        return insert(node);
    }

    // Check if the current area is within the source bounds.
    bool in_src_bounds = offset.x + extent < ctx.params.src.dimensions().x &&
        offset.y + extent < ctx.params.src.dimensions().y &&
        offset.z + extent < ctx.params.src.dimensions().z;

    bool split = false;
    Pixel avg;

    if (const auto* chan_diff_heuristic = std::get_if<ChannelDiffHeuristic>(&ctx.params.heuristic)) {
        auto [avg_pix, max_diff] = ctx.params.src.vol_scan(offset, offset + extent);
        avg = avg_pix;
        split = max_diff > chan_diff_heuristic->channel_diff;
    } else {
        const auto& stddev_heuristic = std::get<StdDevHeuristic>(ctx.params.heuristic);
        auto [avg_pix, stddev] = ctx.params.src.stddev_scan(offset, offset + extent);
        avg = avg_pix;
        split = stddev > stddev_heuristic.stddev;
    }

    if ((!split && in_src_bounds) || extent == 1) {
        // This node is a leaf node
        const auto node = Node{
            .children = {0},
            .color = avg,
            .is_leaf_depth = LEAF | static_cast<uint32_t>(depth),
        };

        return insert(node);
    } else {
        // This node is an intermediary node
        size_t h_extent = extent / 2;
        size_t child = 0;

        auto node = Node{
            .children = {},
            .color = avg,
            .is_leaf_depth = static_cast<uint32_t>(depth),
        };

        for (auto xoff : {size_t{0}, h_extent}) {
            for (auto yoff : {size_t{0}, h_extent}) {
                for (auto zoff : {size_t{0}, h_extent}) {
                    uint32_t index = this->construct(ctx, {offset.x + xoff, offset.y + yoff, offset.z + zoff}, h_extent, depth + 1);
                    node.children[child++] = index;
                }
            }
        }

        return insert(node);
    }
}

template <typename F>
void Octree::walk_leaves_r(F f, const Vec3Sz& pos, size_t extent, size_t depth, Node& node) {
    if (node.is_leaf()) {
        f(pos, extent, depth, node);
        return;
    }

    size_t h_extent = extent / 2;
    size_t child = 0;

    for (auto xoff : {size_t{0}, h_extent}) {
        for (auto yoff : {size_t{0}, h_extent}) {
            for (auto zoff : {size_t{0}, h_extent}) {
                auto child_pos = Vec3Sz{pos.x + xoff, pos.y + yoff, pos.z + zoff};
                auto& child_node = this->nodes[node.children[child++]];
                this->walk_leaves_r(f, child_pos, h_extent, depth + 1, child_node);
            }
        }
    }
}

template <typename F>
void Octree::walk_leaves(F f) {
    this->walk_leaves_r(f, {0, 0, 0}, this->dim, 0, this->nodes[ROOT]);
}

void Octree::generate_ropes() {
    this->walk_leaves([this](const Vec3Sz& pos, size_t extent, size_t depth, Node& node) {
        auto node_xpos = this->find(pos + Vec3Sz{extent, 0, 0}, depth).second;
        auto node_xneg = this->find(pos + Vec3Sz{-extent, 0, 0}, depth).second;

        auto node_ypos = this->find(pos + Vec3Sz{ 0, extent, 0}, depth).second;
        auto node_yneg = this->find(pos + Vec3Sz{ 0, -extent, 0}, depth).second;

        auto node_zpos = this->find(pos + Vec3Sz{0, 0, extent}, depth).second;
        auto node_zneg = this->find(pos + Vec3Sz{0, 0, -extent}, depth).second;

        node.children[0] = static_cast<uint32_t>(node_xpos);
        node.children[1] = static_cast<uint32_t>(node_xneg);

        node.children[2] = static_cast<uint32_t>(node_ypos);
        node.children[3] = static_cast<uint32_t>(node_yneg);

        node.children[4] = static_cast<uint32_t>(node_zpos);
        node.children[5] = static_cast<uint32_t>(node_zneg);
    });
}
