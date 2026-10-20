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
layout(set = 0, binding = 4) uniform sampler2D sphere_texture;
layout(set = 0, binding = 5) uniform sampler2D toon_texture;

layout(set = 1, binding = 0) uniform sampler2D shadow_texture;
layout(location = 4) in vec4 shadow_position;
layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 view_position;
layout(location = 2) in vec3 view_normal;
layout(location = 3) in vec3 view_light;
layout(location = 0) out vec4 out_color;

float shadow_visibility(vec4 light_position, vec3 normal, vec3 light) {
    vec3 projected = light_position.xyz / light_position.w;
    // The shadow vertex pass flips clip Y; image UV = (x/2+1/2, -y/2+1/2).
    vec2 shadow_uv = projected.xy * vec2(0.5, -0.5) + 0.5;
    if (any(lessThan(shadow_uv, vec2(0))) || any(greaterThan(shadow_uv, vec2(1))) ||
        projected.z < 0.0 || projected.z > 1.0) {
        return 1.0;
    }
    ivec2 size = textureSize(shadow_texture, 0);
    if (any(lessThan(size, ivec2(1)))) {
        return 1.0;
    }
    const float MINIMUM_BIAS = 0.0005;
    const float SLOPE_BIAS = 0.002;
    float bias = max(MINIMUM_BIAS, SLOPE_BIAS * (1.0 - max(dot(normal, light), 0.0)));
    ivec2 pixel = ivec2(shadow_uv * vec2(size));
    const int FILTER_RADIUS = 1;
    const float FILTER_SAMPLES = 9.0;
    float lit = 0.0;
    for (int y = -FILTER_RADIUS; y < FILTER_RADIUS + 1; ++y) {
        for (int x = -FILTER_RADIUS; x < FILTER_RADIUS + 1; ++x) {
            ivec2 sample_pixel = pixel + ivec2(x, y);
            if (any(lessThan(sample_pixel, ivec2(0))) || any(greaterThanEqual(sample_pixel, size))) {
                lit += 1.0;
            } else {
                float depth = texelFetch(shadow_texture, sample_pixel, 0).r;
                lit += projected.z - bias < depth ? 1.0 : 0.0;
            }
        }
    }
    // PCF averages nine depth comparisons; ambient illumination is retained by the caller.
    return lit / FILTER_SAMPLES;
}

void main() {
    // Texture morph: sample' = sample * factor + add; Lambert = max(dot(N,L),0).
    vec4 base_color = texture(model_texture, uv) * base_color_texture_factor + base_color_texture_add;
    vec3 normal = normalize(view_normal);
    vec3 light = normalize(view_light);
    float visibility = shadow_visibility(shadow_position, normal, light);
    vec3 lighting = diffuse.rgb * max(dot(normal, light), 0.0) * visibility + ambient;
    vec4 lit_color = vec4(base_color.rgb * clamp(lighting, 0.0, 1.0), base_color.a * diffuse.a);
    vec3 view_direction = normalize(-view_position);
    vec3 reflected = reflect(-view_direction, normal);
    // Sphere UV = (r.x,-r.y)/(2*sqrt(r.x^2+r.y^2+(r.z+1)^2)) + 0.5.
    float multiplier = 2.0 * sqrt(dot(reflected.xy, reflected.xy) + (reflected.z + 1.0) * (reflected.z + 1.0));
    vec2 sphere_uv = vec2(reflected.x / max(multiplier, 0.00001) + 0.5, -reflected.y / max(multiplier, 0.00001) + 0.5);
    vec4 sphere = texture(sphere_texture, sphere_uv) * environment_texture_factor + environment_texture_add;
    vec3 sphere_color = lit_color.rgb * (1.0 - sphere_mul - sphere_add) + (lit_color.rgb * sphere.rgb) * sphere_mul + (lit_color.rgb + sphere.rgb) * sphere_add;
    vec4 toon = texture(toon_texture, vec2(max(dot(normal, light), 0.0), 0.0)) * shading_ramp_texture_factor + shading_ramp_texture_add;
    vec3 half_vector = normalize(-light) + view_direction;
    // Blinn-Phong specular = max(dot(N,normalize(V-L)),0)^shininess.
    float specular_factor = dot(normal, -light) > 0.0 && dot(half_vector, half_vector) > 0.000001 ? pow(max(dot(normal, normalize(half_vector)), 0.0), shininess) : 0.0;
    out_color = vec4(sphere_color * toon.rgb + specular * specular_factor * visibility,
        lit_color.a * toon.a * (1.0 - sphere_mul + sphere_mul * sphere.a));
}
