struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    uint bones : BONEINDICES;
    float2 weights : BONEWEIGHTS;
    float edge_flag : EDGE_FLAG;
};
