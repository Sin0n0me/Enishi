#version 450

layout(set = 0, binding = 2, std140) uniform Material {
    vec4 diffuse; vec3 specular; float shininess; vec3 ambient; float sphere_mul;
    float sphere_add; float edge_flag; vec2 padding;
    vec4 base_color_texture_factor; vec4 base_color_texture_add;
    vec4 environment_texture_factor; vec4 environment_texture_add;
    vec4 shading_ramp_texture_factor; vec4 shading_ramp_texture_add;
    vec4 outline_color; vec4 outline_parameters;
};
layout(set = 0, binding = 3) uniform sampler2D model_texture;
layout(location = 0) in vec2 uv;

void main() {
    const float ALPHA_CUTOFF = 0.5;
    float alpha = (texture(model_texture, uv) * base_color_texture_factor +
        base_color_texture_add).a * diffuse.a;
    if (alpha < ALPHA_CUTOFF) {
        discard;
    }
}
