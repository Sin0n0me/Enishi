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
    const DeformedVertex deformed = deform_vertex(input.position, input.normal, input.bones,
        input.weights, input.skinning_method, input.blend_center, input.blend_anchor0, input.blend_anchor1);
    const float4 skinned_pos = float4(deformed.position, 1.0);
    output.position = apply_shadow_map_light(mul(world, skinned_pos));
    return output;
}
