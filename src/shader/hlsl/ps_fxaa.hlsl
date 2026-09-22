#include "common/constant_buffer/fxaa.hlsl"

Texture2D fxaa_texture;
SamplerState linear_sampler;

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float luminance(const float3 color) {
    return dot(color, float3(0.299, 0.587, 0.114));
}

float4 main(const PSInput input) : SV_TARGET {
    //return fxaa_texture.Sample(linear_sampler, input.uv);
    
    // 
    const float2 texel = inverse_screen_size;
    const float4 rgb_m = fxaa_texture.Sample(linear_sampler, input.uv);
    const float3 rgb_n = fxaa_texture.Sample(linear_sampler, input.uv + float2(0, -texel.y)).rgb;
    const float3 rgb_s = fxaa_texture.Sample(linear_sampler, input.uv + float2(0, texel.y)).rgb;
    const float3 rgb_e = fxaa_texture.Sample(linear_sampler, input.uv + float2(texel.x, 0)).rgb;
    const float3 rgb_w = fxaa_texture.Sample(linear_sampler, input.uv + float2(-texel.x, 0)).rgb;

    // 
    const float luma_m = luminance(rgb_m.rgb);
    const float luma_n = luminance(rgb_n);
    const float luma_s = luminance(rgb_s);
    const float luma_e = luminance(rgb_e);
    const float luma_w = luminance(rgb_w);

    const float luma_min = min(luma_m, min(min(luma_n, luma_s), min(luma_e, luma_w)));
    const float luma_max = max(luma_m, max(max(luma_n, luma_s), max(luma_e, luma_w)));

    float2 dir = float2(
        -((luma_n + luma_s) - (2.0 * luma_m)),
        ((luma_e + luma_w) - (2.0 * luma_m))
    );
    const float dir_reduce = max(
        (luma_n + luma_s + luma_e + luma_w) * (0.25 * fxaa_reduce_mul),
        fxaa_reduce_min
    );
    const float rcp_dir_min = 1.0 / (min(abs(dir.x), abs(dir.y)) + dir_reduce);
    dir = clamp(dir * rcp_dir_min, -fxaa_span_max, fxaa_span_max) * texel;    
    
    const float3 rgb_a = 0.5 * (
        fxaa_texture.Sample(linear_sampler, input.uv + dir * (1.0 / 3.0)).rgb +
        fxaa_texture.Sample(linear_sampler, input.uv + dir * (2.0 / 3.0)).rgb
    );

    const float3 rgb_b = rgb_a * 0.5 + 0.25 * (
        fxaa_texture.Sample(linear_sampler, input.uv + dir * 0.0).rgb +
        fxaa_texture.Sample(linear_sampler, input.uv + dir * 1.0).rgb
    );

    const float3 final_rgb = (rgb_a + rgb_b) * 0.5;
    
    return float4(final_rgb.rgb, rgb_m.a);
}
