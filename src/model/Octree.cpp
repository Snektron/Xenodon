#include "model/Octree.h"
#include <algorithm>
#include "model/VolumetricCube.h"
#include "core/Logger.h"

namespace {
    constexpr const size_t MAX_CHANNEL_DIFFERENCE = 10;

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

    size_t nodes = 0;
    size_t depth = this->construct_r(src, Vec3Sz{0, 0, 0}, dim, nodes);
    LOGGER.log("Total nodes: {}, Depth: {}", nodes, depth);
}

size_t Octree::construct_r(const VolumetricCube& src, Vec3Sz offset, size_t extent, size_t& nodes) {
    ++nodes;

    if (nodes % 100000 == 0) {
        LOGGER.log("{} nodes...", nodes);
    }

    const auto src_dim = src.dimensions();

    const auto bounded_max = Vec3Sz{
        std::min(src_dim.x, offset.x + extent),
        std::min(src_dim.y, offset.y + extent),
        std::min(src_dim.z, offset.z + extent),
    };

    VolumetricCube::Pixel diff;
    if (bounded_max.x == 0 || bounded_max.y == 0 || bounded_max.z == 0) {
        diff = 0;
    } else {
        diff = src.max_diff(offset, bounded_max);
    }

    uint8_t max_diff_channel = *std::max_element(reinterpret_cast<uint8_t*>(&diff), reinterpret_cast<uint8_t*>(&diff) + sizeof(VolumetricCube::Pixel));

    if (max_diff_channel > MAX_CHANNEL_DIFFERENCE) {
        size_t h_extent = extent / 2;
        return 1 + std::max({
            this->construct_r(src, {offset.x, offset.y, offset.z}, h_extent, nodes),
            this->construct_r(src, {offset.x, offset.y, offset.z + h_extent}, h_extent, nodes),
            this->construct_r(src, {offset.x, offset.y + h_extent, offset.z}, h_extent, nodes),
            this->construct_r(src, {offset.x, offset.y + h_extent, offset.z + h_extent}, h_extent, nodes),
            this->construct_r(src, {offset.x + h_extent, offset.y, offset.z}, h_extent, nodes),
            this->construct_r(src, {offset.x + h_extent, offset.y, offset.z + h_extent}, h_extent, nodes),
            this->construct_r(src, {offset.x + h_extent, offset.y + h_extent, offset.z}, h_extent, nodes),
            this->construct_r(src, {offset.x + h_extent, offset.y + h_extent, offset.z + h_extent}, h_extent, nodes),
        });
    } else {
        return 1;
    }
}
