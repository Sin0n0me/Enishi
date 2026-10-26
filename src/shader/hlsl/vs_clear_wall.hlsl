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
    output.shadow = apply_shadow_map_light(mul(world, position));
    output.normal = normalize(mul((float3x3)world, float3(0, 0, -1)));
    return output;
}
