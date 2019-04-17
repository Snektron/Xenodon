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

const int STEPS = 2500;

vec3 ray(vec3 dir, vec3 up, vec2 uv) {
    uv -= 0.5;
    uv.y *= output_region.extent.y / output_region.extent.x;

    vec3 right = normalize(cross(up, dir));
    up = normalize(cross(right, dir));

    return normalize(uv.x * right + uv.y * up + dir);
}

vec3 get_voxel(ivec3 p) {
    return texelFetch(u_model, p, 0).rgb * 0.03;
}

vec3 trace(vec3 ro, vec3 rd, float tfar) {
    ivec3 map_pos = ivec3(floor(ro));
    vec3 delta_dist = abs(1. / rd);
    vec3 sgn = sign(rd);
    ivec3 rs = ivec3(sgn);
    vec3 side_dist = (sgn * (floor(ro) - ro) + (sgn * 0.5) + 0.5) * delta_dist;
    vec3 total = vec3(0);

    float t = 0;

    for (int i = 0; i < STEPS; ++i) {
        total += get_voxel(map_pos);
        bvec3 mask = lessThanEqual(side_dist.xyz, min(side_dist.yzx, side_dist.zxy));
        vec3 fmask = vec3(mask);
        side_dist += fmask * delta_dist;
        map_pos += ivec3(mask) * rs;

        if (any(greaterThanEqual(total, vec3(1)))) {
            break;
        }

        if (map_pos.x < 0 || map_pos.y < 0 || map_pos.z < 0) {
            break;
        } else if (map_pos.x > 2048 || map_pos.y > 2049 || map_pos.z > 48) {
            break;
        }
    }

    return total;
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

void main() {
    vec2 pixel = mix(output_region.min, output_region.max, v_pos);
    vec2 uv = (pixel - output_region.offset) / output_region.extent;

    float t = push.time * 0.2;
    vec3 center = vec3(1024, 1024, 24);
    vec3 ro = center + vec3(sin(t), 0, cos(t)) * 3000.0;
    vec3 rd = normalize(vec3(1024, 1024, 24) - ro);
    rd = ray(rd, vec3(0, 1, 0), uv);

    vec2 hit = aabb_intersect(vec3(0, 0, 0), vec3(2048, 2048, 48), ro, rd);
    if (hit.y > max(hit.x, 0.0)) {
        f_color.xyz = trace(ro + rd * hit.x, rd, hit.y);
        f_color.w = 1;
    } else {
        f_color = vec4(0, 0, 0, 1);
    }
}
