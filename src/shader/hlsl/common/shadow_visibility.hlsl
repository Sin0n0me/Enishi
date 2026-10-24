#ifndef ENISHI_SHADER_BACKEND
#include "common/backend.hlsl"
#endif
ENISHI_BINDING(0, 1) ENISHI_COMBINED
Texture2D<float> shadow_texture : register(t3);
#ifdef ENISHI_SPIRV
ENISHI_BINDING(0, 1) ENISHI_COMBINED SamplerState shadow_sampler : register(s3);
#endif

float shadow_visibility(float4 light_position, float3 normal, float3 light_direction) {
    const float3 projected = light_position.xyz / light_position.w;
    // D3D NDC x/y [-1,1] -> texture [0,1], with the texture Y axis pointing down.
#ifdef ENISHI_OPENGL
    // OpenGL texture Y points up; the light-space depth remains in [0, 1].
    const float2 uv = projected.xy * 0.5f + 0.5f;
#else
    const float2 uv = projected.xy * float2(0.5f, -0.5f) + 0.5f;
#endif
    if (any(uv < 0.0f) || any(uv > 1.0f) || projected.z < 0.0f || projected.z > 1.0f) {
        return 1.0f;
    }
    uint width;
    uint height;
    shadow_texture.GetDimensions(width, height);
    // Missing shadow pass stays lit (e.g. isolated shader validation).
    if (width == 0 || height == 0) {
        return 1.0f;
    }
    static const float MINIMUM_BIAS = 0.0005f;
    static const float SLOPE_BIAS = 0.002f;
    const float bias = max(MINIMUM_BIAS,
        SLOPE_BIAS * (1.0f - saturate(dot(normalize(normal), normalize(light_direction)))));
    const int2 pixel = int2(uv * float2(width, height));
    float lit = 0.0f;
    static const int FILTER_RADIUS = 1;
    static const float FILTER_SAMPLES = 9.0f;
    [unroll]
    for (int y = -FILTER_RADIUS; y < FILTER_RADIUS + 1; ++y) {
        [unroll]
        for (int x = -FILTER_RADIUS; x < FILTER_RADIUS + 1; ++x) {
            const int2 sample_pixel = pixel + int2(x, y);
            if (any(sample_pixel < 0) || any(sample_pixel >= int2(width, height))) {
                lit += 1.0f;
            } else {
                const float depth = shadow_texture.Load(int3(sample_pixel, 0));
                lit += projected.z - bias < depth ? 1.0f : 0.0f;
            }
        }
    }
    // PCF averages depth comparisons, not depth values.
    return lit / FILTER_SAMPLES;
}
