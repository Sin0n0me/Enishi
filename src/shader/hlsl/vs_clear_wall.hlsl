#include "common/input_ps_clear_wall.hlsl"
#include "common/constant_buffer/camera.hlsl"
#include "common/constant_buffer/shadow_map.hlsl"

struct VSInput {
    float3 position : POSITION;
};

PSInput main(const VSInput input) {
    PSInput output;
    const float4 position = float4(input.position, 1.0f);
    output.position = apply_camera(position);
    output.shadow = apply_shadow_map_light(position);
    output.shadow.xyz /= output.shadow.w;
    output.shadow.y *= -1.0f;
    output.shadow.xy = output.shadow.xy * 0.5f + 0.5f;
    
    return output;
}
