#include "common/backend.hlsl"
struct Input { ENISHI_LOCATION(0) float3 position : POSITION; };
struct Output { float4 position : SV_POSITION; ENISHI_LOCATION(0) float2 uv : TEXCOORD0; };
Output main(Input input) {
    Output output;
    output.position = float4(input.position, 1.0);
    output.uv = input.position.xy * float2(0.5, -0.5) + 0.5;
    return output;
}
