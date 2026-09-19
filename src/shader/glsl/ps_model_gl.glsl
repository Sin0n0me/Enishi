#version 400 core

in vec2 uv;
in vec3 normal;

layout(location = 0) out vec4 out_color;

void main() {
    const vec3 light_direction = normalize(vec3(0.0, 0.0, -1.0));
    const float diffuse = max(dot(normalize(normal), -light_direction), 0.15);
    out_color = vec4(vec3(diffuse), 1.0);
}
