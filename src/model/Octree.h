#ifndef _XENODON_MODEL_OCTREE_H
#define _XENODON_MODEL_OCTREE_H

#include <vector>
#include <limits>
#include <filesystem>
#include <array>
#include <cstddef>
#include <cstdint>
#include "math/Vec.h"
#include "model/Pixel.h"
#include "utility/Span.h"

class Grid;

class Octree {
public:
    constexpr const static size_t X_NEG = 0;
    constexpr const static size_t X_POS = 1 << 2;
    constexpr const static size_t Y_NEG = 0;
    constexpr const static size_t Y_POS = 1 << 1;
    constexpr const static size_t Z_NEG = 0;
    constexpr const static size_t Z_POS = 1 << 0;

    constexpr const static uint32_t LEAF = 1u << 31u;

    // This struct should be kept in sync with resources/svo.frag
    struct Node {
        std::array<uint32_t, 8> children;
        Pixel color;
        uint32_t is_leaf_depth;
    };

    static_assert(sizeof(Node) == 40, "");


private:
    struct ConstructionContext;

    size_t dim;
    std::vector<Node> nodes;

    Octree(size_t dim, std::vector<Node>&& nodes);

public:
    Octree(const Grid& src, uint8_t min_channel_diff, bool dag);

    static Octree load_svo(const std::filesystem::path& path);

    void save_svo(const std::filesystem::path& path) const;

    Span<Node> data() const {
        return this->nodes;
    }

    size_t memory_footprint() const {
        return sizeof(Octree) + this->nodes.size() * sizeof(Octree::Node);
    }

    size_t side() const {
        return this->dim;
    }

private:
    uint32_t construct(ConstructionContext& ctx, Vec3Sz offset, size_t extent, size_t depth);
};

#endif
