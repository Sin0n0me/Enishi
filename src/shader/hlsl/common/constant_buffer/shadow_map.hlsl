#ifndef ENISHI_SHADER_BACKEND
#include "common/backend.hlsl"
#endif

ENISHI_BINDING(6, 0)
cbuffer ShadowMap : register(b2) {
    column_major matrix light_view_proj;
    float4 shadow_options;
};

float4 apply_shadow_map_light(float4 position) {    
    position = mul(light_view_proj, position);
    return position;
}
