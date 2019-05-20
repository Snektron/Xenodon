#include "model/Octree.h"
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include <string_view>
#include <cmath>
#include <fmt/format.h>
#include "core/Logger.h"
#include "core/Error.h"
#include "model/Grid.h"
#include "utility/serialization.h"

namespace {
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

    constexpr const std::string_view SVO_FMT_ID = "XNDN-SVO";
}

template<>
struct std::hash<Octree::Node> {
    size_t operator()(const Octree::Node& node) const {
        size_t v = std::hash<uint32_t>{}(node.is_leaf_depth);
        v = hash_combine(v, std::hash<uint32_t>{}(node.color.pack()));

        for (uint32_t child : node.children) {
            v = hash_combine(v, std::hash<uint32_t>{}(child));
        }

        return v;
    }
};

bool operator==(const Octree::Node& lhs, const Octree::Node& rhs) {
    if (lhs.is_leaf_depth != rhs.is_leaf_depth || lhs.color != rhs.color)
        return false;

    return std::equal(std::begin(lhs.children), std::end(lhs.children), std::begin(rhs.children));
}

bool operator!=(const Octree::Node& lhs, const Octree::Node& rhs) {
    return !(lhs == rhs);
}

struct Octree::ConstructionContext {
    const Grid& src;
    std::unordered_map<Node, uint32_t> map = {};
    uint8_t min_channel_diff;
    bool dag;
};

Octree::Octree(size_t dim, std::vector<Node>&& nodes):
    dim(dim), nodes(std::move(nodes)) {
}

Octree::Octree(const Grid& src, uint8_t min_channel_diff, bool dag) {
    const auto src_dim = src.dimensions();
    this->dim = std::max({ceil_2pow(src_dim.x), ceil_2pow(src_dim.y), ceil_2pow(src_dim.z)});

    auto ctx = ConstructionContext{
        .src = src,
        .min_channel_diff = min_channel_diff,
        .dag = dag
    };

    this->construct(ctx, Vec3Sz{0, 0, 0}, this->dim, 0);

    this->nodes.shrink_to_fit();
    std::reverse(this->nodes.begin(), this->nodes.end());

    uint32_t end = static_cast<uint32_t>(this->nodes.size()) - 1;
    for (auto& node : this->nodes) {
        for (uint32_t& child : node.children) {
            child = end - child;
        }
    }
}

Octree Octree::load_svo(const std::filesystem::path& path) {
    auto in = std::ifstream(path, std::ios::binary);
    if (!in) {
        throw Error("Failed to open");
    }

    char id[SVO_FMT_ID.size()];
    in.read(id, SVO_FMT_ID.size());
    if (SVO_FMT_ID != std::string_view(id, SVO_FMT_ID.size())) {
        fmt::print("Fmt id: '{}', got: '{}'\n", SVO_FMT_ID, std::string_view(id, SVO_FMT_ID.size()));
        throw Error("Invalid format id");
    }

    uint64_t dim = read_uint_le<uint64_t>(in);
    uint64_t num_nodes = read_uint_le<uint64_t>(in);

    auto pos = in.tellg();
    if (pos == std::ifstream::pos_type(-1)) {
        throw Error("Failed to tell");
    }

    in.seekg(0, std::ios_base::end);
    auto end = in.tellg();
    if (end == std::ifstream::pos_type(-1)) {
        throw Error("Failed to tell");
    }

    in.seekg(pos);
    size_t remaining = static_cast<size_t>(end - pos);
    if (remaining != sizeof(Node) * num_nodes) {
        throw Error("File size does not match number of nodes");
    }

    auto nodes = std::vector<Node>(num_nodes);
    for (auto& node : nodes) {
        for (uint32_t& child : node.children) {
            child = read_uint_le<uint32_t>(in);
        }

        node.color = Pixel::unpack(read_uint_le<uint32_t>(in));
        node.is_leaf_depth = read_uint_le<uint32_t>(in);
    }

    return Octree(static_cast<size_t>(dim), std::move(nodes));
}

void Octree::save_svo(const std::filesystem::path& path) const {
    auto out = std::ofstream(path, std::ios::binary);
    if (!out) {
        throw Error("Failed to open");
    }

    out.write(SVO_FMT_ID.data(), SVO_FMT_ID.size());
    write_uint_le(out, this->dim);
    write_uint_le(out, this->nodes.size());

    for (const auto& node : this->nodes) {
        for (uint32_t child : node.children) {
            write_uint_le(out, child);
        }

        write_uint_le(out, node.color.pack());
        write_uint_le(out, node.is_leaf_depth);
    }
}

const Octree::Node* Octree::find(const Vec3Sz& pos, size_t max_depth) const {
    size_t extent = this->dim;
    if (pos.x > extent || pos.y > extent || pos.z > extent) {
        return nullptr;
    }

    size_t index = 0;
    auto offset = Vec3Sz{0, 0, 0};

    while (true) {
        extent /= 2;

        if (this->nodes[index].is_leaf_depth & LEAF || extent == 0 || max_depth == 0) {
            return &this->nodes[index];
        }

        size_t child_index = 0;

        if (pos.x >= offset.x + extent) {
            child_index |= X_POS;
            offset.x += extent;
        }

        if (pos.y >= offset.y + extent) {
            child_index |= Y_POS;
            offset.y += extent;
        }

        if (pos.z >= offset.z + extent) {
            child_index |= Z_POS;
            offset.z += extent;
        }

        index = this->nodes[index].children[child_index];

        --max_depth;
    }
}

Vec3F Octree::test_trace(const Vec3F& ro, Vec3F rd, bool debug) const {
    constexpr const size_t float_mantissa_size = 23;
    constexpr const float epsilon = 0.0001f;

    if (std::fabs(rd.x) < epsilon) {
        rd.x = std::copysignf(epsilon, rd.x);
    }

    if (std::fabs(rd.y) < epsilon) {
        rd.y = std::copysignf(epsilon, rd.y);
    }

    if (std::fabs(rd.z) < epsilon) {
        rd.z = std::copysignf(epsilon, rd.z);
    }

    const Vec3F rrd = 1.0 / rd;
    const Vec3F bias = rrd * ro;

    size_t stack_index = float_mantissa_size - 1;

    std::array<uint32_t, float_mantissa_size + 1> node_stack;
    std::array<uint32_t, float_mantissa_size + 1> child_index_stack;
    std::array<float, float_mantissa_size + 1> side_stack;

    node_stack[float_mantissa_size] = 0;
    child_index_stack[float_mantissa_size] = 8;

    uint32_t parent = 0;
    uint32_t idx = 0;
    Vec3F pos = {1, 1, 1};
    float side = 0.5;

    Vec3F total_color = {0, 0, 0};

    const auto max_elem = [](const Vec3F& x) {
        return std::max({x.x, x.y, x.z});
    };

    const auto min_elem = [](const Vec3F& x) {
        return std::min({x.x, x.y, x.z});
    };

    const auto max = [](const Vec3F& x, const Vec3F& y) {
        return Vec3F {
            std::max(x.x, y.x),
            std::max(x.y, y.y),
            std::max(x.z, y.z)
        };
    };

    const auto min = [](const Vec3F& x, const Vec3F& y) {
        return Vec3F {
            std::min(x.x, y.x),
            std::min(x.y, y.y),
            std::min(x.z, y.z)
        };
    };

    size_t i = 0;

    while (stack_index < float_mantissa_size && i < 10000) {
        const uint32_t child = this->nodes.at(parent).children[idx];

        const Vec3F box_min = pos * rrd - bias;
        const Vec3F box_max = (pos + side) * rrd - bias;

        const float t_min = max_elem(min(box_min, box_max));
        const float t_max = min_elem(max(box_min, box_max));

        const auto indent = std::string((float_mantissa_size - stack_index - 1) * 2, ' ');

        ++i;

        if (debug) {
            fmt::print("{}parent: {}, child: {}, idx: {}, leaf: {} :: {}\n", indent, parent, child, idx, this->nodes.at(child).is_leaf_depth >= LEAF, t_min <= t_max ? "hit" : "miss");
        }

        if (t_min <= t_max) {
            if (this->nodes.at(child).is_leaf_depth >= LEAF) {
                Vec3F color = {
                    static_cast<float>(this->nodes[child].color.r) / 255.f,
                    static_cast<float>(this->nodes[child].color.g) / 255.f,
                    static_cast<float>(this->nodes[child].color.b) / 255.f
                };

                total_color += color * (t_max - t_min);
            } else {
                if (idx != 7) {
                    node_stack.at(stack_index) = parent;
                    child_index_stack.at(stack_index) = idx;
                    side_stack.at(stack_index) = side;
                    --stack_index;
                }

                side *= 0.5f;

                parent = child;
                idx = 0;
                continue;
            }
        }

        if (idx == 7) {
            ++stack_index;

            if (stack_index >= float_mantissa_size) {
                return total_color;
            }

            if (debug) {
                fmt::print("{}pop {} -> ", indent, idx);
            }

            parent = node_stack.at(stack_index);
            idx = child_index_stack.at(stack_index);
            side = side_stack.at(stack_index);

            if (debug) {
                fmt::print("{}\n", idx);
            }
        }

        pos = Vec3F {
            pos.x - std::fmod(pos.x, side * 2.f),
            pos.y - std::fmod(pos.y, side * 2.f),
            pos.z - std::fmod(pos.z, side * 2.f),
        };

        ++idx;

        if (idx & 4) {
            pos.x += side;
        }

        if (idx & 2) {
            pos.y += side;
        }

        if (idx & 1) {
            pos.z += side;
        }
    }

    return total_color;
}

uint32_t Octree::construct(ConstructionContext& ctx, Vec3Sz offset, size_t extent, size_t depth) {
    auto insert = [&](const Node& node) {
        uint32_t node_index = static_cast<uint32_t>(this->nodes.size());

        if (ctx.dag) {
            auto [it, inserted] = ctx.map.insert({node, node_index});
            if (inserted) {
                this->nodes.push_back(node);
            }

            return it->second;
        } else {
            this->nodes.push_back(node);
            return node_index;
        }
    };

    auto [avg, max_diff] = ctx.src.vol_scan(offset, offset + extent);

    if (max_diff <= ctx.min_channel_diff || extent == 1) {
        // This node is a leaf node
        const auto node = Node{
            .children = {0},
            .color = avg,
            .is_leaf_depth = LEAF | static_cast<uint32_t>(depth),
        };

        return insert(node);
    } else {
        // This node is an intermediary node
        size_t h_extent = extent / 2;
        size_t child = 0;

        auto node = Node{
            .children = {},
            .color = avg,
            .is_leaf_depth = static_cast<uint32_t>(depth),
        };

        for (auto xoff : {size_t{0}, h_extent}) {
            for (auto yoff : {size_t{0}, h_extent}) {
                for (auto zoff : {size_t{0}, h_extent}) {
                    uint32_t index = this->construct(ctx, {offset.x + xoff, offset.y + yoff, offset.z + zoff}, h_extent, depth + 1);
                    node.children[child++] = index;
                }
            }
        }

        return insert(node);
    }
}
