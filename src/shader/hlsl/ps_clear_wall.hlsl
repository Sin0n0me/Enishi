#include "common/input_ps_clear_wall.hlsl"
#include "common/shadow_visibility.hlsl"
#include "common/shadow_light.hlsl"

float4 main(const PSInput input) : SV_TARGET {
    static const float SHADOW_OPACITY = 0.25f;
    const float visibility = shadow_visibility(input.shadow, input.normal, SHADOW_LIGHT_DIRECTION);
    // Only occluded pixels contribute alpha; unshadowed wall pixels remain fully transparent.
    return float4(0, 0, 0, (1.0f - visibility) * SHADOW_OPACITY);
}
