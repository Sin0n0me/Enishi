#version 400 core
layout(location = 0) in vec3 in_position;
layout(std140) uniform Camera {
    mat4 world; mat4 view; mat4 projection; mat4 mvp;
};
layout(std140) uniform ShadowMap {
    mat4 light_view_proj;
};
out vec4 shadow_position;
out vec3 world_normal;
void main() {
    vec4 position = vec4(in_position, 1.0);
    gl_Position = mvp * position;
    shadow_position = light_view_proj * world * position;
    world_normal = normalize(mat3(world) * vec3(0, 0, -1));
}
