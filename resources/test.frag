#version 450

layout(location = 0) in vec3 v_color;
layout(location = 1) in vec2 v_pos;

layout(location = 0) out vec4 f_color;

const float ASPECT = 600. / 800.;
const float EPSILON = 0.001;
const int STEPS = 100;
const vec3 LIGHT = normalize(vec3(1, 2, 3));

float sphere(in vec3 p, in float r) {
    return length(p) - r;
}

float box(vec3 p, vec3 b) {
    vec3 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
}

vec3 op_repeat(in vec3 p, in vec3 c) {
    return mod(p, c)- 0.5 * c;
}

float op_sub(float d1, float d2) {
    return max(d1, -d2);
}

float field(in vec3 p) {
    p = op_repeat(p, vec3(5.));
    return op_sub(box(p, vec3(1)), sphere(p, 1.4));
}

float march(in vec3 ro, in vec3 rd) {
    float t = 0.0;

    for (int i = 0; i < STEPS; ++i) {
        vec3 p = ro + t * rd;
        float d = field(p);
        if (d < EPSILON) {
            return t;
        }

        t += d;
    }

    return -1.0;
}

vec3 nabla(in vec3 p) {
    float d0 = field(p);
    float dX = field(p - vec3(EPSILON, 0.0, 0.0));
    float dY = field(p - vec3(0.0, EPSILON, 0.0));
    float dZ = field(p - vec3(0.0, 0.0, EPSILON));
    return -normalize(vec3(dX - d0, dY - d0, dZ - d0));
}

vec3 ray(in vec3 dir, in vec3 up) {
    vec2 uv = v_pos * 0.5;
    uv.y *= ASPECT;

    vec3 right = normalize(cross(up, dir));
    up = normalize(cross(right, dir));

    return normalize(uv.x * right + uv.y * up + dir);
}

void main() {
    vec3 ro = vec3(0, 0, 5);
    vec3 rd = ray(normalize(-ro), vec3(0, 1, 0));

    float dst = march(ro, rd);
    vec3 p = ro + dst * rd;
    vec3 normal = nabla(p);

    if (dst >= 0.0)
        f_color.xyz = vec3(dot(normal, LIGHT) * 0.5 + 0.5) / (dst * 0.1);
    else
        f_color.xyz = vec3(0);
    f_color.w = 1.0;
}
