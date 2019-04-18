#include "model/Octree.h"
#include <algorithm>
#include <chrono>
#include <fmt/chrono.h>
#include "model/Grid.h"
#include "core/Logger.h"

namespace {
    constexpr const uint8_t MIN_CHANNEL_DIFF = 25;

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

Octree::Octree(const Grid& src) {
    const auto src_dim = src.dimensions();
    this->dim = std::max({ceil_2pow(src_dim.x), ceil_2pow(src_dim.y), ceil_2pow(src_dim.z)});

    LOGGER.log("Source size: {}x{}x{}", src_dim.x, src_dim.y, src_dim.z);
    LOGGER.log("Octree size: {}x{}x{}", this->dim, this->dim, this->dim);

    auto map = std::unordered_map<Node, uint32_t>();
    const auto start = std::chrono::high_resolution_clock::now();
    this->construct(src, map, Vec3Sz{0, 0, 0}, this->dim, 0);
    const auto stop = std::chrono::high_resolution_clock::now();

    LOGGER.log("Total nodes: {:n}", this->nodes.size());
    LOGGER.log("Construction took: {}", std::chrono::duration<double>(stop - start));
    LOGGER.log("Total unique nodes: {:n}", map.size());

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

uint32_t Octree::construct(const Grid& src, std::unordered_map<Node, uint32_t>& map, Vec3Sz offset, size_t extent, size_t depth) {
    if (this->nodes.size() % 1'000'000 == 0 && this->nodes.size() > 0) {
        LOGGER.log(
            "{:n} nodes... ({} GiB)",
            this->nodes.size(),
            static_cast<float>(this->nodes.size() * sizeof(Node)) / (1024.f * 1024.f * 1024.f)
        );
    }

    auto insert = [&](const Node& node) {
        uint32_t node_index = static_cast<uint32_t>(this->nodes.size());
        auto [it, inserted] = map.insert({node, node_index});

        if (inserted) {
            this->nodes.push_back(node);
        }

        return it->second;
    };

    auto [avg, max_diff] = src.vol_scan(offset, offset + extent);

    if (max_diff <= MIN_CHANNEL_DIFF || extent == 1) {
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
                    uint32_t index = this->construct(src, map, {offset.x + xoff, offset.y + yoff, offset.z + zoff}, h_extent, depth + 1);
                    node.children[child++] = index;
                }
            }
        }

        return insert(node);
    }
}
