#include "model/Octree.h"
#include <algorithm>
#include "model/Grid.h"
#include "core/Logger.h"

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
}

template<>
struct std::hash<Octree::Node> {
    size_t operator()(const Octree::Node& node) const {
        size_t v = std::hash<uint32_t>{}(node.is_leaf_depth);
        v = hash_combine(v, std::hash<uint32_t>{}(node.color.pack()));

        for (size_t i = 0; i < 8; ++i) {
            v = hash_combine(v, std::hash<uint32_t>{}(node.children[i]));
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
    const Grid& src;
    std::unordered_map<Node, uint32_t> map;
    uint8_t min_channel_diff;
    bool dag;
};

Octree::Octree(const Grid& src, uint8_t min_channel_diff, bool dag) {
    const auto src_dim = src.dimensions();
    this->dim = std::max({ceil_2pow(src_dim.x), ceil_2pow(src_dim.y), ceil_2pow(src_dim.z)});

    auto ctx = ConstructionContext{
        .src = src,
        .min_channel_diff = min_channel_diff,
        .dag = dag
    };

    this->construct(ctx, Vec3Sz{0, 0, 0}, this->dim, 0);

    this->nodes.shrink_to_fit();
    std::reverse(this->nodes.begin(), this->nodes.end());

    uint32_t end = static_cast<uint32_t>(this->nodes.size()) - 1;
    for (auto& node : this->nodes) {
        for (uint32_t& child : node.children) {
            child = end - child;
        }
    }
}

const Octree::Node* Octree::find(const Vec3Sz& pos, size_t max_depth) const {
    size_t extent = this->dim;
    if (pos.x > extent || pos.y > extent || pos.z > extent) {
        return nullptr;
    }

    size_t index = 0;
    auto offset = Vec3Sz{0, 0, 0};

    while (true) {
        extent /= 2;

        if (this->nodes[index].is_leaf_depth & LEAF || extent == 0 || max_depth == 0) {
            return &this->nodes[index];
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
    auto insert = [&](const Node& node) {
        uint32_t node_index = static_cast<uint32_t>(this->nodes.size());

        if (ctx.dag) {
            auto [it, inserted] = ctx.map.insert({node, node_index});
            if (inserted) {
                this->nodes.push_back(node);
            }

            return it->second;
        } else {
            this->nodes.push_back(node);
            return node_index;
        }
    };

    auto [avg, max_diff] = ctx.src.vol_scan(offset, offset + extent);

    if (max_diff <= ctx.min_channel_diff || extent == 1) {
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
