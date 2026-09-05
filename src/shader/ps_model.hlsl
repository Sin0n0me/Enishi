#include "common/input_ps_model.hlsl"
#include "common/constant_buffer/material.hlsl"

Texture2D model_texture;
Texture2D sphere_texture;
Texture2D toon_texture;
SamplerState model_sampler;
SamplerState sphere_sampler;
SamplerState toon_sampler;


float4 apply_sphere(
    const float4 color,
    const float3 position,
    const float3 normal
) {
    const float2 sphere_uv = calc_sphere_uv(position, normal);
    const float3 sphere_color = sphere_texture.Sample(sphere_sampler, sphere_uv).rgb;
    
    return apply_sphere_map(color, sphere_color);
}

float4 apply_toon(
    const float4 color,
    const float3 normal,
    const float3 light_direction
) {
    const float dot_nl = saturate(dot(normalize(normal), light_direction));
    const float2 toon_uv = float2(dot_nl, 0.0f);
    const float3 toon_color = toon_texture.Sample(toon_sampler, toon_uv).rgb;

    return color * float4(toon_color, 1.0);
}

float4 main(const PSInput input) : SV_TARGET {    
    const float4 base_color = model_texture.Sample(model_sampler, input.uv);
    
    // 光の適用
    // TODO: 光源位置や色を定数バッファで指定できるように
   
    const float3 light_color = float3(1.0, 1.0, 1.0);
    const float4 lighting_color = apply_lighting(
        base_color,
        light_color,
        input.view_light,
        input.view_normal
    );
    
    // スフィアの適用
    const float4 sphere_color = apply_sphere(
        lighting_color,
        input.view_position,
        input.view_normal
    );
    
    // トゥーンの適用
    const float4 toon_color = apply_toon(
        sphere_color,
        input.view_normal,
        input.view_light
    );
        
    // スペキュラー反射の適用
    const float4 specular_color = apply_specular(
        toon_color,
        input.view_position,
        input.view_light,
        input.view_normal
    );
    
    const float4 final_color = specular_color;
    
    return final_color;
}
