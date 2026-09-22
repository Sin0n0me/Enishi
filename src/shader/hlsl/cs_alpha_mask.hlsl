#include "common/constant_buffer/alpha_mask_params.hlsl"

Texture2D<float4> input_alpha_mask;
RWTexture2D<uint> output_alpha_mask;

[numthreads(16, 16, 1)]
void main(const uint3 id : SV_DispatchThreadID) {   
    const float4 color = input_alpha_mask.Load(int3(id.xy, 0));
    const uint result = (color.a > alpha_threshold) ? 1u : 0u;
    output_alpha_mask[id.xy] = result;
}
