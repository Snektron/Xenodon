#version 450

// This struct should be kept in sync with
// src/model/Octree.cpp Octree::Node
struct Node {
    uint children[8];
    uint color;
    uint is_leaf_depth;
};

layout(push_constant) uniform PushConstant {
    float time;
} push;

layout(binding = 0) uniform OutputRegionBuffer {
    vec2 min;
    vec2 max;
    vec2 offset;
    vec2 extent;
} output_region;

layout(binding = 1) readonly buffer Octree {
    Node nodes[];
};

layout(location = 0) in vec3 v_color;
layout(location = 1) in vec2 v_pos;

layout(location = 0) out vec4 f_color;

const uint LEAF = 1 << 31;
const float HALF_VOXEL_SIZE = 1.0 / 2048.0 / 2.0;

vec4 unpack(uint v) {
    return vec4(
        float(v & 0xFF) / 255.0,
        float((v >> 8) & 0xFF) / 255.0,
        float((v >> 16) & 0xFF) / 255.0,
        float((v >> 24) & 0xFF) / 255.0
    );
}

vec3 ray(vec3 dir, vec3 up, vec2 uv) {
    uv -= 0.5;
    uv.y *= output_region.extent.y / output_region.extent.x;

    vec3 right = normalize(cross(up, dir));
    up = normalize(cross(right, dir));

    return normalize(uv.x * right + uv.y * up + dir);
}

vec2 aabb_intersect(vec3 bmin, vec3 bmax, vec3 ro, vec3 rd) {
    vec3 tbot = (bmin - ro) / rd;
    vec3 ttop = (bmax - ro) / rd;
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    vec2 t = max(tmin.xx, tmin.yz);
    float t0 = max(t.x, t.y);
    t = min(tmax.xx, tmax.yz);
    float t1 = min(t.x, t.y);
    return vec2(t0, t1);
}

uint find(vec3 pos, out vec3 base, out float side) {
    float extent = 1.0;

    uint index = 0;
    vec3 offset = vec3(0);

    while (true) {
        if (nodes[index].is_leaf_depth >= LEAF) {
            base = offset;
            side = extent;
            return index;
        }

        extent *= 0.5;
        bvec3 mask = greaterThanEqual(pos, offset + extent);
        int child = int(dot(vec3(mask), vec3(4, 2, 1)));
        offset += vec3(mask) * vec3(extent);
        index = nodes[index].children[child];
    }
}

vec4 trace(vec3 ro, vec3 rd) {
    vec2 t = aabb_intersect(vec3(0), vec3(1), ro, rd);
    if (t.y < 0 || t.x > t.y) {
        return vec4(0);
    }

    t.x = max(t.x, 0);

    vec4 total = vec4(0);

    for (int i = 0; i < 500 && t.x + HALF_VOXEL_SIZE < t.y; ++i) {
        vec3 p = (t.x + HALF_VOXEL_SIZE) * rd + ro;
        vec3 offset;
        float side;
        uint node = find(p, offset, side);

        vec2 s = aabb_intersect(offset, offset + side, ro, rd);
        t.x = s.y;

        vec4 color = unpack(nodes[node].color);
        total += color * (s.y - s.x) * 40.0;
    }

    return total;
}

void main() {
    vec2 pixel = mix(output_region.min, output_region.max, v_pos);
    vec2 uv = (pixel - output_region.offset) / output_region.extent;

    float t = push.time * 0.2;
    vec3 center = vec3(0.5, 0.5, 0.5);
    vec3 ro = center + vec3(sin(t), 0, cos(t)) * 2;
    vec3 rd = normalize(center - ro);
    rd = ray(rd, vec3(0, 1, 0), uv);

    vec4 color = trace(ro, rd);
    f_color = vec4(color.rgb, 1);
}
