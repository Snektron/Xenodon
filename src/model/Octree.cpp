#include "model/Octree.h"
#include <algorithm>
#include <chrono>
#include <fmt/chrono.h>
#include "model/VolumetricCube.h"
#include "core/Logger.h"

namespace {
    constexpr const uint8_t MIN_CHANNEL_DIFF = 40;

    // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    size_t ceil_2pow(size_t x) {
        --x;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        x |= x >> 32;
        return ++x;
    }
}

Octree::Octree(const VolumetricCube& src) {
    const auto src_dim = src.dimensions();
    size_t dim = std::max({ceil_2pow(src_dim.x), ceil_2pow(src_dim.y), ceil_2pow(src_dim.z)});

    LOGGER.log("Source size: {}x{}x{}", src_dim.x, src_dim.y, src_dim.z);
    LOGGER.log("Octree size: {}x{}x{}", dim, dim, dim);

    const auto start = std::chrono::high_resolution_clock::now();
    this->construct(src, Vec3Sz{0, 0, 0}, dim);
    const auto stop = std::chrono::high_resolution_clock::now();

    LOGGER.log("Total nodes: {}", this->nodes.size());
    LOGGER.log("Construction took: {}", std::chrono::duration<double>(stop - start));
}

uint32_t Octree::construct(const VolumetricCube& src, Vec3Sz offset, size_t extent) {
    if (this->nodes.size() % 100000 == 0) {
        LOGGER.log(
            "{} nodes... ({} GiB)",
            this->nodes.size(),
            static_cast<float>(this->nodes.size() * sizeof(Node)) / (1024.f * 1024.f * 1024.f)
        );
    }

    auto [avg, max_diff] = src.vol_scan(offset, offset + extent);

    uint32_t node_index = static_cast<uint32_t>(this->nodes.size());

    this->nodes.emplace_back(Node{});
    this->nodes[node_index].color = avg;

    if (max_diff <= MIN_CHANNEL_DIFF || extent == 2) {
        // This node is a leaf node
        for (size_t i = 0; i < 8; ++i) {
            this->nodes[node_index].children[i] = 0;
        }

        this->nodes[node_index].is_leaf = 1;
    } else {
        // This node is an intermediary node

        size_t h_extent = extent / 2;
        size_t child = 0;

        for (auto xoff : {size_t{0}, h_extent}) {
            for (auto yoff : {size_t{0}, h_extent}) {
                for (auto zoff : {size_t{0}, h_extent}) {
                    uint32_t index = this->construct(src, {offset.x + xoff, offset.y + yoff, offset.z + zoff}, h_extent);
                    this->nodes[node_index].children[child++] = index;
                }
            }
        }

        this->nodes[node_index].is_leaf = 0;
    }

    return node_index;
}
