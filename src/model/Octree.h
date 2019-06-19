#ifndef _XENODON_MODEL_OCTREE_H
#define _XENODON_MODEL_OCTREE_H

#include <vector>
#include <limits>
#include <filesystem>
#include <array>
#include <utility>
#include <functional>
#include <cstddef>
#include <cstdint>
#include "math/Vec.h"
#include "model/Pixel.h"
#include "utility/Span.h"

class Octree {
public:
    enum class Type {
        Sparse,
        Dag,
        Rope
    };

    constexpr const static size_t X_NEG = 0;
    constexpr const static size_t X_POS = 1 << 2;
    constexpr const static size_t Y_NEG = 0;
    constexpr const static size_t Y_POS = 1 << 1;
    constexpr const static size_t Z_NEG = 0;
    constexpr const static size_t Z_POS = 1 << 0;

    constexpr const static uint32_t LEAF = 1u << 31u;
    constexpr const static size_t ROOT = 0;

    // This struct should be kept in sync with resources/svo.frag
    struct Node {
        std::array<uint32_t, 8> children;
        Pixel color;
        uint32_t is_leaf_depth;

        bool is_leaf() const {
            return (this->is_leaf_depth & LEAF) != 0;
        }
    };

    static_assert(sizeof(Node) == 40, "Compiler didnt pack Node struct properly");

private:
    size_t dim;
    std::vector<Node> nodes;

public:
    Octree(size_t dim, std::vector<Node>&& nodes);

    static Octree load_svo(const std::filesystem::path& path);

    void save_svo(const std::filesystem::path& path) const;

    std::pair<const Octree::Node*, size_t> find(const Vec3Sz& pos, size_t max_depth) const;

    void generate_ropes();

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
    template <typename F>
    void walk_leaves_r(F f, const Vec3Sz& pos, size_t extent, size_t depth, Node& node);

    template <typename F>
    void walk_leaves(F f);
};

template<>
struct std::hash<Octree::Node> {
    size_t operator()(const Octree::Node& node) const;
};

bool operator==(const Octree::Node& lhs, const Octree::Node& rhs);

#endif
