#version 400 core
uniform sampler2D shadow_texture;
in vec4 shadow_position;
in vec3 world_normal;
layout(location = 0) out vec4 out_color;
float shadow_visibility(vec4 light_position, vec3 normal, vec3 light) {
    vec3 projected = light_position.xyz / light_position.w;
    // FBO textures and GL NDC both point up in Y; unlike D3D there is no Y inversion.
    vec2 shadow_uv = projected.xy * 0.5 + 0.5;
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
    const float SHADOW_OPACITY = 0.25;
    const vec3 LIGHT_DIRECTION = normalize(vec3(-0.35, 0.8, -1.0));
    float visibility = shadow_visibility(shadow_position, normalize(world_normal), LIGHT_DIRECTION);
    // Alpha = shadow coverage * opacity; the wall itself contributes neither color nor opacity.
    out_color = vec4(0, 0, 0, (1.0 - visibility) * SHADOW_OPACITY);
}
