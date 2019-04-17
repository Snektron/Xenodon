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

vec4 unpack(uint v) {
    return vec4(
        float(v & 0xFF) / 255.0,
        float((v >> 8) & 0xFF) / 255.0,
        float((v >> 16) & 0xFF) / 255.0,
        float((v >> 24) & 0xFF) / 255.0
    );
}

bool is_leaf(uint node) {
    return (nodes[node].is_leaf_depth & LEAF) == 1;
}

vec3 ray(vec3 dir, vec3 up, vec2 uv) {
    uv -= 0.5;
    uv.y *= output_region.extent.y / output_region.extent.x;

    vec3 right = normalize(cross(up, dir));
    up = normalize(cross(right, dir));

    return normalize(uv.x * right + uv.y * up + dir);
}

uint first_node(vec3 t0, vec3 tm) {
    uint node = 0;

    if (t0.x > max(t0.y, t0.z)) {
        // YZ plane
        if (tm.y < t0.x) {
            node |= 1 << 1;
        }
        if (tm.z < t0.x) {
            node |= 1 << 2;
        }
    } else if (t0.y > t0.z) {
        // ZX plane
        if (tm.x < t0.y) {
            node |= 1 << 0;
        }
        if (tm.z < t0.y) {
            node |= 1 << 2;
        }
    } else {
        // XY Plane
        if (tm.x < t0.z) {
            node |= 1 << 0;
        }
        if (tm.y < t0.z) {
            node |= 1 << 1;
        }
    }

    return node;
}

uint new_node(vec3 v, ivec3 states) {
    if (v.x > max(v.y, v.z)) {
        return states[0];
    } else if (v.y > v.z) {
        return states[1];
    } else {
        return states[2];
    }
}

struct State {
    vec3 t0, t1, tm;
    uint node;
    uint state;
};

vec4 subtrace(vec3 t0, vec3 t1, uint a) {
    State[30] stack;
    int sp = 0;

    stack[0].t0 = t0;
    stack[0].t1 = t1;
    stack[0].node = 0;

    uint state = 9;

    vec4 total = vec4(0);

    while (sp >= 0) {
        switch (state) {
            case 0: {
                uint old = sp++;
                vec3 t0 = stack[old].t0;
                vec3 t1 = stack[old].tm;
                stack[old].state = new_node(t1, ivec3(4, 2, 1));
                stack[sp].t0 = t0;
                stack[sp].t1 = t1;
                stack[sp].node = nodes[stack[old].node].children[a];

                state = 9;
                break;
            }
            case 1: {
                uint old = sp++;
                vec3 t0 = vec3(stack[old].t0.x, stack[old].t0.y, stack[old].tm.z);
                vec3 t1 = vec3(stack[old].tm.x, stack[old].tm.y, stack[old].t1.z);
                stack[old].state = new_node(t1, ivec3(5, 3, 8));
                stack[sp].t0 = t0;
                stack[sp].t1 = t1;
                stack[sp].node = nodes[stack[old].node].children[a ^ 1];

                state = 9;
                break;
            }
            case 2: {
                uint old = sp++;
                vec3 t0 = vec3(stack[old].t0.x, stack[old].tm.y, stack[old].t0.z);
                vec3 t1 = vec3(stack[old].tm.x, stack[old].t1.y, stack[old].tm.z);
                stack[old].state = new_node(t1, ivec3(6, 8, 3));
                stack[sp].t0 = t0;
                stack[sp].t1 = t1;
                stack[sp].node = nodes[stack[old].node].children[a ^ 2];

                state = 9;
                break;
            }
            case 3: {
                uint old = sp++;
                vec3 t0 = vec3(stack[old].t0.x, stack[old].tm.y, stack[old].tm.z);
                vec3 t1 = vec3(stack[old].tm.x, stack[old].t1.y, stack[old].t1.z);
                stack[old].state = new_node(t1, ivec3(7, 8, 8));
                stack[sp].t0 = t0;
                stack[sp].t1 = t1;
                stack[sp].node = nodes[stack[old].node].children[a ^ 3];

                state = 9;
                break;
            }
            case 4: {
                uint old = sp++;
                vec3 t0 = vec3(stack[old].tm.x, stack[old].t0.y, stack[old].t0.z);
                vec3 t1 = vec3(stack[old].t1.x, stack[old].tm.y, stack[old].tm.z);
                stack[old].state = new_node(t1, ivec3(8, 6, 5));
                stack[sp].t0 = t0;
                stack[sp].t1 = t1;
                stack[sp].node = nodes[stack[old].node].children[a ^ 4];

                state = 9;
                break;
            }
            case 5: {
                uint old = sp++;
                vec3 t0 = vec3(stack[old].tm.x, stack[old].tm.y, stack[old].t0.z);
                vec3 t1 = vec3(stack[old].t1.x, stack[old].t1.y, stack[old].tm.z);
                stack[old].state = new_node(t1, ivec3(8, 7, 8));
                stack[sp].t0 = t0;
                stack[sp].t1 = t1;
                stack[sp].node = nodes[stack[old].node].children[a ^ 5];

                state = 9;
                break;
            }
            case 6: {
                uint old = sp++;
                vec3 t0 = vec3(stack[old].tm.x, stack[old].tm.y, stack[old].t0.z);
                vec3 t1 = vec3(stack[old].t1.x, stack[old].t1.y, stack[old].tm.z);
                stack[old].state = new_node(t1, ivec3(8, 8, 7));
                stack[sp].t0 = t0;
                stack[sp].t1 = t1;
                stack[sp].node = nodes[stack[old].node].children[a ^ 6];

                state = 9;
                break;
            }
            case 7: {
                uint old = sp++;
                vec3 t0 = stack[old].tm;
                vec3 t1 = stack[old].t1;
                stack[old].state = 8;
                stack[sp].t0 = t0;
                stack[sp].t1 = t1;
                stack[sp].node = nodes[stack[old].node].children[a ^ 7];

                state = 9;
                break;
            }
            case 8: {
                state = stack[--sp].state;
                break;
            }
            case 9: {
                if (any(lessThan(stack[sp].t1, vec3(0)))) {
                    state = stack[--sp].state;
                    break;
                }

                if (is_leaf(stack[sp].node) || sp >= 0) {
                    state = stack[--sp].state;
                    total += 0.01;
                    break;
                }

                stack[sp].tm = mix(stack[sp].t0, stack[sp].t1, 0.5);
                state = first_node(stack[sp].t0, stack[sp].tm);
                break;
            }
        }
    }

    return total;
}

vec4 trace(vec3 ro, vec3 rd) {
    uint a = 0;

    if (rd.x < 0) {
        ro.x = 1 - ro.x;
        rd.x = -rd.x;
        a |= 4;
    }

    if (rd.y < 0) {
        ro.y = 1 - ro.y;
        rd.y = -rd.y;
        a |= 2;
    }

    if (rd.z < 0) {
        ro.z = 1 - ro.z;
        rd.z = -rd.z;
        a |= 1;
    }

    vec3 t0 = -ro / rd;
    vec3 t1 = (1 - ro) / rd;

    if (max(max(t0.x, t0.y), t0.z) < min(min(t1.x, t1.y), t1.z)) {
        return subtrace(t0, t1, a);
    } else {
        return vec4(1, 0, 1, 1);
    }
}

void main() {
    vec2 pixel = mix(output_region.min, output_region.max, v_pos);
    vec2 uv = (pixel - output_region.offset) / output_region.extent;

    vec3 ro = vec3(1.5, 1.5, 3);
    vec3 rd = normalize(vec3(0.5) - ro);
    rd = ray(rd, vec3(0, 1, 0), uv);

    vec4 color = trace(ro, rd);
    f_color = vec4(color.rgb, 1);
}
