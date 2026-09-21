#version 400 core

layout(std140) uniform Material { vec4 diffuse; vec3 specular; float shininess; vec3 ambient; float sphere_mul; float sphere_add; float edge_flag; vec2 padding; };
uniform sampler2D model_texture;
uniform sampler2D sphere_texture;
uniform sampler2D toon_texture;

in vec2 uv;
in vec3 view_position;
in vec3 view_normal;
in vec3 view_light;
layout(location = 0) out vec4 out_color;

void main() {
    const vec4 base_color = texture(model_texture, uv);
    const vec3 normal = normalize(view_normal);
    const vec3 light = normalize(view_light);
    const vec3 lighting = diffuse.rgb * max(dot(normal, light), 0.0) + ambient;
    const vec4 lit_color = vec4(base_color.rgb * clamp(lighting, 0.0, 1.0), base_color.a);
    const vec3 view_direction = normalize(-view_position);
    const vec3 reflected = reflect(-view_direction, normal);
    const float multiplier = 2.0 * sqrt(dot(reflected.xy, reflected.xy) + (reflected.z + 1.0) * (reflected.z + 1.0));
    const vec2 sphere_uv = vec2(reflected.x / max(multiplier, 0.00001) + 0.5, -reflected.y / max(multiplier, 0.00001) + 0.5);
    const vec3 sphere = texture(sphere_texture, sphere_uv).rgb;
    const vec3 sphere_color = lit_color.rgb * (1.0 - sphere_mul - sphere_add) + (lit_color.rgb * sphere) * sphere_mul + (lit_color.rgb + sphere) * sphere_add;
    const vec3 toon = texture(toon_texture, vec2(max(dot(normal, light), 0.0), 0.0)).rgb;
    const vec3 half_vector = normalize(-light) + view_direction;
    const float specular_factor = dot(normal, -light) > 0.0 && dot(half_vector, half_vector) > 0.000001 ? pow(max(dot(normal, normalize(half_vector)), 0.0), shininess) : 0.0;
    out_color = vec4(sphere_color * toon + specular * specular_factor, lit_color.a);
}
