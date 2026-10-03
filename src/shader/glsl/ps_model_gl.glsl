#version 400 core

layout(std140) uniform Material {
    vec4 diffuse; vec3 specular; float shininess; vec3 ambient; float sphere_mul;
    float sphere_add; float edge_flag; vec2 padding;
    vec4 base_color_texture_factor; vec4 base_color_texture_add;
    vec4 environment_texture_factor; vec4 environment_texture_add;
    vec4 shading_ramp_texture_factor; vec4 shading_ramp_texture_add;
    vec4 outline_color; vec4 outline_parameters;
};
uniform sampler2D model_texture;
uniform sampler2D sphere_texture;
uniform sampler2D toon_texture;

in vec2 uv;
in vec3 view_position;
in vec3 view_normal;
in vec3 view_light;
layout(location = 0) out vec4 out_color;

void main() {
    vec4 base_color = texture(model_texture, uv) * base_color_texture_factor + base_color_texture_add;
    vec3 normal = normalize(view_normal);
    vec3 light = normalize(view_light);
    vec3 lighting = diffuse.rgb * max(dot(normal, light), 0.0) + ambient;
    vec4 lit_color = vec4(base_color.rgb * clamp(lighting, 0.0, 1.0), base_color.a * diffuse.a);
    vec3 view_direction = normalize(-view_position);
    vec3 reflected = reflect(-view_direction, normal);
    float multiplier = 2.0 * sqrt(dot(reflected.xy, reflected.xy) + (reflected.z + 1.0) * (reflected.z + 1.0));
    vec2 sphere_uv = vec2(reflected.x / max(multiplier, 0.00001) + 0.5, -reflected.y / max(multiplier, 0.00001) + 0.5);
    vec4 sphere = texture(sphere_texture, sphere_uv) * environment_texture_factor + environment_texture_add;
    vec3 sphere_color = lit_color.rgb * (1.0 - sphere_mul - sphere_add) + (lit_color.rgb * sphere.rgb) * sphere_mul + (lit_color.rgb + sphere.rgb) * sphere_add;
    vec4 toon = texture(toon_texture, vec2(max(dot(normal, light), 0.0), 0.0)) * shading_ramp_texture_factor + shading_ramp_texture_add;
    vec3 half_vector = normalize(-light) + view_direction;
    float specular_factor = dot(normal, -light) > 0.0 && dot(half_vector, half_vector) > 0.000001 ? pow(max(dot(normal, normalize(half_vector)), 0.0), shininess) : 0.0;
    out_color = vec4(sphere_color * toon.rgb + specular * specular_factor,
        lit_color.a * toon.a * (1.0 - sphere_mul + sphere_mul * sphere.a));
}
