#ifndef _XENODON_MODEL_OCTREE_H
#define _XENODON_MODEL_OCTREE_H

#include <vector>
#include <cstddef>
#include <cstdint>
#include "math/Vec.h"

class VolumetricCube;

class Octree {
    struct Node {
        uint64_t children[8];
    };

    std::vector<Node> data;

public:
    explicit Octree(const VolumetricCube& src);

private:
    size_t construct_r(const VolumetricCube& src, Vec3Sz offset, size_t extent, size_t& nodes);
};

#endif
