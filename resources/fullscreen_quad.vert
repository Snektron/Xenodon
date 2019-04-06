#version 450

layout(location = 0) out vec3 v_color;
layout(location = 1) out vec2 v_pos;

const vec3 colors[] = vec3[](
    vec3(0, 1, 0),
    vec3(0, 0, 1),
    vec3(1, 0, 0),
    vec3(1, 0, 1)
);

void main() {
    v_color = colors[gl_VertexIndex];
    v_pos = vec2(
        1.0 - step(2, gl_VertexIndex),
        1.0 - mod(gl_VertexIndex, 2)
    );

    gl_Position = vec4(v_pos * 2 - 1, 0, 1);
}
