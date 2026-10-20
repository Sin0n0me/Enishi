#include "common/input_vs_model.hlsl"
#include "common/constant_buffer/camera.hlsl"
#include "common/constant_buffer/bones.hlsl"
#include "common/constant_buffer/shadow_map.hlsl"

struct ShadowInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

ShadowInput main(const VSInput input) {
    const DeformedVertex deformed = deform_vertex(input.position, input.normal, input.bones,
        input.weights, input.skinning_method, input.blend_center, input.blend_anchor0, input.blend_anchor1);
    ShadowInput output;
    output.position = apply_shadow_map_light(mul(world, float4(deformed.position, 1.0f)));
    output.uv = input.uv;
    return output;
}
