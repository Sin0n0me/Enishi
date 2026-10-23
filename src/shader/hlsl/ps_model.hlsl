#include "common/input_ps_model.hlsl"
#include "common/constant_buffer/material.hlsl"
#include "common/shadow_visibility.hlsl"

ENISHI_BINDING(3, 0) ENISHI_COMBINED Texture2D model_texture : register(t0);
ENISHI_BINDING(4, 0) ENISHI_COMBINED Texture2D sphere_texture : register(t1);
ENISHI_BINDING(5, 0) ENISHI_COMBINED Texture2D toon_texture : register(t2);
ENISHI_BINDING(3, 0) ENISHI_COMBINED SamplerState model_sampler : register(s0);
ENISHI_BINDING(4, 0) ENISHI_COMBINED SamplerState sphere_sampler : register(s1);
ENISHI_BINDING(5, 0) ENISHI_COMBINED SamplerState toon_sampler : register(s2);


float4 apply_sphere(
    const float4 color,
    const float3 position,
    const float3 normal
) {
    const float2 sphere_uv = calc_sphere_uv(position, normal);
    const float4 sphere_color = sphere_texture.Sample(sphere_sampler, sphere_uv) * environment_texture_factor + environment_texture_add;
    
    return apply_sphere_map(color, sphere_color);
}

float4 apply_toon(
    const float4 color,
    const float3 normal,
    const float3 light_direction
) {
    const float dot_nl = saturate(dot(normalize(normal), light_direction));
    const float2 toon_uv = float2(dot_nl, 0.0f);
    const float4 toon_color = toon_texture.Sample(toon_sampler, toon_uv) * shading_ramp_texture_factor + shading_ramp_texture_add;

    return color * toon_color;
}

float4 main(const PSInput input) : SV_TARGET {    
    const float4 base_color = model_texture.Sample(model_sampler, input.uv) * base_color_texture_factor + base_color_texture_add;
    
    // 光の適用
    // TODO: 光源位置や色を定数バッファで指定できるように
   
    const float visibility = shadow_visibility(
        input.shadow_position, input.view_normal, input.view_light);
    const float3 light_color = float3(visibility, visibility, visibility);
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
    
    // Retain ambient illumination and alpha; shadow only the direct specular contribution.
    const float4 final_color = float4(
        toon_color.rgb + (specular_color.rgb - toon_color.rgb) * visibility, toon_color.a);
    
    return final_color;
}
