#include "common/input_vs_model.hlsl"
#include "common/input_ps_model.hlsl"
#include "common/constant_buffer/camera.hlsl"
#include "common/constant_buffer/bones.hlsl"

PSInput main(const VSInput input) {
    PSInput output;
    const float4 local_pos = float4(input.position, 1.0f);
        
    const float3 light_direction = normalize(
        float3(0.0f, 0.0f, -10.0f)
    );
    
    // スキニング
    const float4 skinned_pos = skinned_position(local_pos, input.bones, input.weights);
    const float4 clip_pos = apply_camera(skinned_pos);
    const float3 skinned_nor = skinned_normal(input.normal, input.bones, input.weights);    
    const matrix mv = view * world;
    const float3 view_pos = mul(mv, skinned_pos).xyz;
    const float3 view_nor = normalize(mul((float3x3)mv, skinned_nor));
    const float3 view_light = mul((float3x3)mv, light_direction);
    
    output.position = clip_pos;
    output.normal = skinned_nor;
    output.uv = input.uv;
    output.view_position = view_pos;
    output.view_normal = view_nor;
    output.view_light = view_light;
    
    return output;
}
