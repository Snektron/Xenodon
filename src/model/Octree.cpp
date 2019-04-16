#include "model/Octree.h"
#include <algorithm>
#include <chrono>
#include <fmt/chrono.h>
#include "model/Grid.h"
#include "core/Logger.h"

namespace {
    constexpr const uint8_t MIN_CHANNEL_DIFF = 0;

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
        size_t v = std::hash<uint32_t>{}(node.is_leaf);
        v = hash_combine(v, std::hash<uint32_t>{}(node.color));

        for (size_t i = 0; i < 8; ++i) {
            v = hash_combine(v, std::hash<uint32_t>{}(node.children[i]));
        }

        return v;
    }
};

bool operator==(const Octree::Node& lhs, const Octree::Node& rhs) {
    if (lhs.is_leaf != rhs.is_leaf || lhs.color != rhs.color)
        return false;

    for (size_t i = 0; i < 8; ++i) {
        if (lhs.children[i] != rhs.children[i])
            return false;
    }

    return true;
}

bool operator!=(const Octree::Node& lhs, const Octree::Node& rhs) {
    return !(lhs == rhs);
}

Octree::Octree(const Grid& src) {
    const auto src_dim = src.dimensions();
    size_t dim = std::max({ceil_2pow(src_dim.x), ceil_2pow(src_dim.y), ceil_2pow(src_dim.z)});

    LOGGER.log("Source size: {}x{}x{}", src_dim.x, src_dim.y, src_dim.z);
    LOGGER.log("Octree size: {}x{}x{}", dim, dim, dim);

    auto map = std::unordered_map<Node, uint32_t>();
    const auto start = std::chrono::high_resolution_clock::now();
    this->construct(src, map, Vec3Sz{0, 0, 0}, dim);
    const auto stop = std::chrono::high_resolution_clock::now();

    LOGGER.log("Total nodes: {}", this->nodes.size());
    LOGGER.log("Construction took: {}", std::chrono::duration<double>(stop - start));
}

uint32_t Octree::construct(const Grid& src, std::unordered_map<Node, uint32_t>& map, Vec3Sz offset, size_t extent) {
    if (this->nodes.size() % 100000 == 0) {
        LOGGER.log(
            "{} nodes... ({} GiB)",
            this->nodes.size(),
            static_cast<float>(this->nodes.size() * sizeof(Node)) / (1024.f * 1024.f * 1024.f)
        );
    }

    auto [avg, max_diff] = src.vol_scan(offset, offset + extent);


    if (max_diff <= MIN_CHANNEL_DIFF || extent == 2) {
        // This node is a leaf node
        uint32_t node_index = static_cast<uint32_t>(this->nodes.size());
        auto& node = this->nodes.emplace_back(Node{
            .children = {0},
            .color = avg,
            .is_leaf = 1
        });

        map.insert({node, node_index});

        return node_index;
    } else {
        // This node is an intermediary node

        size_t h_extent = extent / 2;
        size_t child = 0;

        auto node = Node{
            .children = {},
            .color = avg,
            .is_leaf = 0
        };

        for (auto xoff : {size_t{0}, h_extent}) {
            for (auto yoff : {size_t{0}, h_extent}) {
                for (auto zoff : {size_t{0}, h_extent}) {
                    uint32_t index = this->construct(src, map, {offset.x + xoff, offset.y + yoff, offset.z + zoff}, h_extent);
                    node.children[child++] = index;
                }
            }
        }

        uint32_t node_index = static_cast<uint32_t>(this->nodes.size());
        auto [it, inserted] = map.insert({node, node_index});
        if (inserted) {
            return node_index;
        } else {
            return it->second;
        }
    }
}
