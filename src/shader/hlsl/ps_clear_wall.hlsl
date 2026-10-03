#include "common/input_ps_clear_wall.hlsl"

Texture2D<float> shadow_map;
SamplerComparisonState shadow_sampler;

float4 main(const PSInput input) : SV_TARGET {
    const float depth = input.shadow.z;
    const float shadow = shadow_map.SampleCmpLevelZero(
        shadow_sampler, 
        input.shadow.xy,
        depth
    );
       
    // ある程度影は透過させる
    const float alpha = (1.0f - shadow) * 0.25;
    
    return float4(0.0f, 0.0f, 0.0f, alpha);
}
