#include "common/backend.hlsl"
ENISHI_BINDING(2, 0) cbuffer Crack {
    float4 view_depth_opacity; // xy: tangent-space view slope, z: UV depth scale, w: opacity
    float4 texel_size; // xy: reciprocal mask dimensions
};
ENISHI_BINDING(3, 0) ENISHI_COMBINED Texture2D model_texture : register(t0);
ENISHI_BINDING(3, 0) ENISHI_COMBINED SamplerState model_sampler : register(s0);
struct Input { float4 position : SV_POSITION; ENISHI_LOCATION(0) float2 uv : TEXCOORD0; };
static const int MIN_STEPS = 12;
static const int MAX_STEPS = 48;
static const float EPSILON = 0.00001;
static const float3 LIGHT_DIRECTION = float3(-0.4, -0.6, 0.7);
static const float3 GROOVE_COLOR = float3(0.12, 0.17, 0.22);
static const float3 RIM_COLOR = float3(0.75, 0.85, 0.95);
static const float SPECULAR_EXPONENT = 24;
static const float RECESS_DARKENING = 0.7;
static const float SPECULAR_INTENSITY = 0.65;
float depth_at(float2 uv) { return model_texture.SampleLevel(model_sampler, saturate(uv), 0).r; }
float2 parallax_uv(float2 uv) {
    float2 offset = view_depth_opacity.xy * view_depth_opacity.z;
    if (dot(offset, offset) < EPSILON * EPSILON || depth_at(uv) < EPSILON) { return uv; }
    int steps = (int)lerp((float)MIN_STEPS, (float)MAX_STEPS, saturate(length(view_depth_opacity.xy)));
    float step_depth = 1.0 / (float)steps;
    float layer = 0;
    float previous_delta = depth_at(uv);
    float2 current = uv;
    [loop] for (int index = 0; index < MAX_STEPS; ++index) {
        if (index == steps) { break; }
        layer += step_depth;
        current = uv - offset * layer;
        float delta = depth_at(current) - layer;
        if (delta < 0 || abs(delta) < EPSILON) {
            // Linear root of the height/ray difference between the last two samples.
            float fraction = previous_delta / max(previous_delta - delta, EPSILON);
            return lerp(current + offset * step_depth, current, saturate(fraction));
        }
        previous_delta = delta;
    }
    return current;
}
float4 main(Input input) : SV_TARGET {
    float2 uv = parallax_uv(input.uv);
    float2 field = model_texture.SampleLevel(model_sampler, saturate(uv), 0).rg;
    float2 dx = float2(texel_size.x, 0);
    float2 dy = float2(0, texel_size.y);
    // Central differences of the recessed height h = -depth * scale.
    float slope_x = (depth_at(uv + dx) - depth_at(uv - dx)) * view_depth_opacity.z / (2 * texel_size.x);
    float slope_y = (depth_at(uv + dy) - depth_at(uv - dy)) * view_depth_opacity.z / (2 * texel_size.y);
    float3 normal = normalize(float3(slope_x, slope_y, 1));
    float3 light = normalize(LIGHT_DIRECTION);
    float diffuse = saturate(dot(normal, light));
    float highlight = pow(saturate(dot(reflect(-light, normal), normalize(float3(view_depth_opacity.xy, 1)))), SPECULAR_EXPONENT);
    float3 color = lerp(GROOVE_COLOR, RIM_COLOR, diffuse);
    color *= 1 - RECESS_DARKENING * field.r;
    color += highlight * SPECULAR_INTENSITY;
    return float4(saturate(color), saturate(field.g * view_depth_opacity.w));
}
