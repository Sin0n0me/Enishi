#version 450
layout(location = 0) in vec3 in_position;
layout(set = 0, binding = 0, std140) uniform Camera {
    mat4 world; mat4 view; mat4 projection; mat4 mvp;
};
layout(set = 0, binding = 6, std140) uniform ShadowMap {
    mat4 light_view_proj;
};
layout(location = 0) out vec4 shadow_position;
layout(location = 1) out vec3 world_normal;
void main() {
    vec4 position = vec4(in_position, 1.0);
    gl_Position = mvp * position;
    // Main camera projection is GL-style; Vulkan requires Y inversion and [0,w] depth.
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;
    shadow_position = light_view_proj * world * position;
    world_normal = normalize(mat3(world) * vec3(0, 0, -1));
}
