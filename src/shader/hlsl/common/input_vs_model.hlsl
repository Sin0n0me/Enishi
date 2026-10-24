#ifndef ENISHI_SHADER_BACKEND
#include "common/backend.hlsl"
#endif
struct VSInput {
    ENISHI_LOCATION(0) float3 position : POSITION;
    ENISHI_LOCATION(1) float3 normal : NORMAL;
    ENISHI_LOCATION(2) float2 uv : TEXCOORD;
    ENISHI_LOCATION(3) uint4 bones : BONEINDICES;
    ENISHI_LOCATION(4) float4 weights : BONEWEIGHTS;
    ENISHI_LOCATION(9) float edge_flag : EDGE_FLAG;
    ENISHI_LOCATION(5) uint skinning_method : SKINNING_METHOD;
    ENISHI_LOCATION(6) float3 blend_center : BLEND_CENTER;
    ENISHI_LOCATION(7) float3 blend_anchor0 : BLEND_ANCHOR_A;
    ENISHI_LOCATION(8) float3 blend_anchor1 : BLEND_ANCHOR_B;
};
