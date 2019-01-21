#version 450

layout(location = 0) out vec3 v_color;

const vec2 positions[] = vec2[](
    vec2(1.0, 1.0),
    vec2(1.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(-1.0, -1.0)
);

const vec3 colors[] = vec3[](
    vec3(0, 0, 0),
    vec3(0, 0, 1),
    vec3(1, 0, 0),
    vec3(1, 0, 1)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    v_color = colors[gl_VertexIndex];
}
