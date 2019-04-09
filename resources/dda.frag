#version 450

layout(push_constant) uniform PushConstant {
    float time;
} push;

layout(binding = 0) uniform OutputRegionBuffer {
    vec2 min;
    vec2 max;
    vec2 offset;
    vec2 extent;
} output_region;

layout(binding = 1) uniform sampler3D u_model;

layout(location = 0) in vec3 v_color;
layout(location = 1) in vec2 v_pos;

layout(location = 0) out vec4 f_color;

const vec3 LIGHT = normalize(vec3(1, 2, 3));
const int STEPS = 1000;

vec3 ray(vec3 dir, vec3 up, vec2 uv) {
    uv -= 0.5;
    uv.y *= output_region.extent.y / output_region.extent.x;
    uv *= 2.0;

    vec3 right = normalize(cross(up, dir));
    up = normalize(cross(right, dir));

    return normalize(uv.x * right + uv.y * up + dir);
}

float get_voxel(ivec3 p) {
    if (p.x < 0 || p.y < 0 || p.z < 0)
        return 0;
    else if (p.x > 2048 || p.y > 2048 || p.z > 48)
        return 0;

    return texelFetch(u_model, p, 0).g * 0.1;
}

vec3 trace(vec3 ro, vec3 rd) {
    ivec3 map_pos = ivec3(floor(ro));
    vec3 delta_dist = abs(vec3(length(rd)) / rd);
    ivec3 rs = ivec3(sign(rd));
    vec3 side_dist = (sign(rd) * (vec3(map_pos) - ro) + (sign(rd) * 0.5) + 0.5) * delta_dist;
    float total = 0.0;

    for (int i = 0; i < STEPS; i++) {
        float v = get_voxel(map_pos);
        bvec3 mask = lessThanEqual(side_dist.xyz, min(side_dist.yzx, side_dist.zxy));
        side_dist += vec3(mask) * delta_dist;
        map_pos += ivec3(mask) * rs;
        total += v * dot(vec3(mask) * delta_dist, vec3(1));
        if (total > 1.0)
            break;
    }

    return vec3(total);
}

void main() {
    vec2 pixel = mix(output_region.min, output_region.max, v_pos);
    vec2 uv = (pixel - output_region.offset) / output_region.extent;

    vec3 ro = vec3(1024, 1024, -300);
    vec3 rd = normalize(vec3(0, 0, 1));
    rd = ray(rd, vec3(0, 1, 0), uv);
    f_color.xyz = trace(ro, rd);
    f_color.w = 1.0;
}
