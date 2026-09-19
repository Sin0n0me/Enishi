#include "common/input_vs_model.hlsl"
#include "common/constant_buffer/camera.hlsl"
#include "common/constant_buffer/bones.hlsl"
#include "common/constant_buffer/shadow_map.hlsl"

struct VSOutput {
    float4 position : SV_POSITION;
};

VSOutput main(const VSInput input) {
    VSOutput output;
    const float4 local_pos = float4(input.position, 1.0f);
    const float4 skinned_pos = skinned_position(local_pos, input.bones, input.weights);
    output.position = apply_shadow_map_light(mul(world, skinned_pos));
    return output;
}
