struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    uint4 bones : BONEINDICES;
    float4 weights : BONEWEIGHTS;
    float edge_flag : EDGE_FLAG;
    uint skinning_method : SKINNING_METHOD;
    float3 blend_center : BLEND_CENTER;
    float3 blend_anchor0 : BLEND_ANCHOR_A;
    float3 blend_anchor1 : BLEND_ANCHOR_B;
};
