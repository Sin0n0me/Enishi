#include "common/constant_buffer/material.hlsl"
Texture2D model_texture : register(t0);
SamplerState model_sampler : register(s0);

void main(float4 position : SV_POSITION, float2 uv : TEXCOORD0) {
    static const float ALPHA_CUTOFF = 0.5f;
    const float alpha = (model_texture.Sample(model_sampler, uv) *
        base_color_texture_factor + base_color_texture_add).a * diffuse.a;
    // A depth-only pass still runs the pixel shader so transparent texels cannot cast solid shadows.
    clip(alpha - ALPHA_CUTOFF);
}
