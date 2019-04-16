#ifndef _XENODON_MODEL_OCTREE_H
#define _XENODON_MODEL_OCTREE_H

#include <vector>
#include <unordered_map>
#include <cstddef>
#include <cstdint>
#include "math/Vec.h"

class Grid;

class Octree {
    struct Node {
        uint32_t children[8];
        uint32_t color;
        uint32_t is_leaf;
    };

    std::vector<Node> nodes;

public:
    explicit Octree(const Grid& src);

private:
    uint32_t construct(const Grid& src, std::unordered_map<Node, uint32_t>& map, Vec3Sz offset, size_t extent);

    friend struct std::hash<Octree::Node>;
    friend bool operator==(const Node&, const Node&);
    friend bool operator!=(const Node&, const Node&);
};

#endif
