#ifndef ENISHI_SHADER_BACKEND
#include "common/backend.hlsl"
#endif
struct PSInput {
    float4 position : SV_POSITION;
    ENISHI_LOCATION(0) float2 uv : TEXCOORD0;
    ENISHI_LOCATION(5) float3 normal : NORMAL;
    ENISHI_LOCATION(1) float3 view_position : VIEW_POSITION;
    ENISHI_LOCATION(2) float3 view_normal : VIEW_NORMAL;
    ENISHI_LOCATION(3) float3 view_light : VIEW_LIGHT;
    ENISHI_LOCATION(4) float4 shadow_position : TEXCOORD1;
};
