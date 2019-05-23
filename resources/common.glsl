#ifndef _XENODON_RAYTRACE_SHADER_GLSL
#define _XENODON_RAYTRACE_SHADER_GLSL

// Define common structures, bindings and functions that every shader needs

struct Camera {
    vec4 forward;
    vec4 up;
    vec4 translation;
};

struct Rect {
    ivec2 offset;
    uvec2 extent;
};

struct RenderParameters {
    vec4 voxel_ratio;
    uvec4 model_dim;
    float density;
};

layout(local_size_x = 8, local_size_y = 8) in;

layout(push_constant) uniform PushConstant {
    Camera camera;
} push;

layout(binding = 0) readonly uniform UniformBuffer {
    Rect output_region;
    Rect display_region;
    RenderParameters params;
} uniforms;

layout(binding = 1, rgba8) restrict writeonly uniform image2D render_target;

const uint FLOAT_MANTISSA_BITS = 23;

// Adjust the ray direction so that no component is zero
vec3 adjust_ray(vec3 rd) {
    const float epsilon = exp2(-float(FLOAT_MANTISSA_BITS));
    return mix(rd, step(rd, vec3(0)) * epsilon * 2.0 - epsilon, lessThan(abs(rd), vec3(epsilon)));
}

vec3 ray(vec2 uv) {
    uv -= 0.5;
    uv.y *= float(uniforms.display_region.extent.y) / float(uniforms.display_region.extent.x);

    vec3 dir = push.camera.forward.xyz;
    vec3 up = push.camera.up.xyz;
    vec3 right = normalize(cross(up, dir));
    up = normalize(cross(right, dir));

    vec3 rd = normalize(uv.x * right + uv.y * up + dir);
    return adjust_ray(normalize(rd / uniforms.params.voxel_ratio.xyz));
}

float min_elem(vec3 v) {
    return min(v.x, min(v.y, v.z));
}

float max_elem(vec3 v) {
    return max(v.x, max(v.y, v.z));
}

// Calculate the density of a voxel for some ray. The base
// density is multiplied by the amount the ray is stretched
float voxel_density(vec3 rd) {
    vec3 rd2 = rd * rd;
    vec3 dim2 = uniforms.params.voxel_ratio.xyz * uniforms.params.voxel_ratio.xyz;
    return uniforms.params.density * sqrt(dot(rd2, dim2) / dot(rd2, vec3(1)));
}

#endif
