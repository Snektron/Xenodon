#ifndef _XENODON_MODEL_OCTREE_H
#define _XENODON_MODEL_OCTREE_H

#include <vector>
#include <cstddef>
#include <cstdint>
#include "math/Vec.h"

class VolumetricCube;

class Octree {
    struct Node {
        uint32_t children[8];
        uint32_t color;
        uint32_t is_leaf;
    };

    std::vector<Node> nodes;

public:
    explicit Octree(const VolumetricCube& src);

private:
    uint32_t construct(const VolumetricCube& src, Vec3Sz offset, size_t extent);
};

#endif
