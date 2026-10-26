#ifndef ENISHI_SHADER_BACKEND
#include "common/backend.hlsl"
#endif
struct PSInput {
    float4 position : SV_POSITION;
    ENISHI_LOCATION(0) float4 shadow : TEXCOORD0;
    ENISHI_LOCATION(1) float3 normal : TEXCOORD1;
};
