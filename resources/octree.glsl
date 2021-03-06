#ifndef _XENODON_OCTREE_GLSL
#define _XENODON_OCTREE_GLSL

// Define structures, bindings and constants for octree raytracing shaders

struct Node {
    uint children[8];
    uint color;
    uint is_leaf_depth;
};

layout(binding = 2) readonly buffer Octree {
    Node nodes[];
} model;

const uint LEAF_MASK = 1 << 31;
const uint DEPTH_MASK = 0x7FFFFFFF;

#endif
