#ifndef _XENODON_MODEL_OCTREE_H
#define _XENODON_MODEL_OCTREE_H

#include <vector>
#include <unordered_map>
#include <limits>
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
        uint32_t children[8];
        Pixel color;
        uint32_t is_leaf_depth;
    };

    static_assert(sizeof(Node) == 40, "");

private:
    struct ConstructionContext;

    std::vector<Node> nodes;
    size_t dim;

public:
    Octree(const Grid& src, uint8_t min_channel_diff, bool dag);

    const Node* find(const Vec3Sz& pos, size_t max_depth = std::numeric_limits<size_t>::max()) const;

    Span<Node> data() const {
        return this->nodes;
    }

    size_t memory_footprint() const {
        return sizeof(Octree) + this->nodes.size() * sizeof(Octree::Node);
    }

private:
    uint32_t construct(ConstructionContext& ctx, Vec3Sz offset, size_t extent, size_t depth);
};

#endif
