#version 400 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in uvec2 in_bone_indices;
layout(location = 4) in vec2 in_bone_weights;

layout(std140) uniform Camera {
    mat4 world;
    mat4 view;
    mat4 projection;
    mat4 mvp;
};

layout(std140) uniform Bones {
    mat4 bone_matrices[512];
};

out vec2 uv;
out vec3 view_position;
out vec3 view_normal;
out vec3 view_light;

void main() {
    const vec3 light_direction = normalize(vec3(0.0, 0.0, -10.0));
    const mat4 skin = bone_matrices[in_bone_indices.x] * in_bone_weights.x +
        bone_matrices[in_bone_indices.y] * in_bone_weights.y;
    const vec4 position = skin * vec4(in_position, 1.0);
    const mat4 model_view = view * world;
    gl_Position = mvp * position;
    uv = in_uv;
    view_position = (model_view * position).xyz;
    view_normal = normalize(mat3(model_view) * (mat3(skin) * in_normal));
    view_light = mat3(model_view) * light_direction;
}
