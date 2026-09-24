#include "common/input_vs_model.hlsl"
#include "common/input_ps_model_edge.hlsl"
#include "common/constant_buffer/model_edge.hlsl"
#include "common/constant_buffer/camera.hlsl"
#include "common/constant_buffer/bones.hlsl"

PSInput main(const VSInput input) {
    const float4 position = float4(input.position, 1.0f);

    // スキニング
    const DeformedVertex deformed = deform_vertex(input.position, input.normal, input.bones,
        input.weights, input.skinning_method, input.blend_center, input.blend_anchor0, input.blend_anchor1);
    const float4 skinned_pos = float4(deformed.position, 1.0);
    const float3 skinned_nor = normalize(deformed.normal);

    // ビュー空間で法線の方向に押し出す
    const float4 view_pos = mul(view, skinned_pos);
    const float3 view_nor = normalize(mul((float3x3)view, skinned_nor));
    float4 clip_pos = mul(proj, view_pos);
    clip_pos.xy += view_nor.xy * edge_width * clip_pos.w;

    PSInput output;
    output.position = clip_pos;
    output.edge_flag = input.edge_flag;
 
    return output;
}
