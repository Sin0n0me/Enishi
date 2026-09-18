struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 view_position : VIEW_POSITION;
    float3 view_normal : VIEW_NORMAL;
    float3 view_light : VIEW_LIGHT;
};