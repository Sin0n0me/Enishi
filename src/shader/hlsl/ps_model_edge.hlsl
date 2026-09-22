#include "common/input_ps_model_edge.hlsl"
#include "common/constant_buffer/model_edge.hlsl"
#include "common/constant_buffer/material.hlsl"

float4 main(const PSInput input) : SV_TARGET {
    return float4(edge_color, input.edge_flag * edge_flag);
}
