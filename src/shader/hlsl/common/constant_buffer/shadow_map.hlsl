cbuffer ShadowMap {
    column_major matrix light_view_proj;
};

float4 apply_shadow_map_light(float4 position) {    
    position = mul(light_view_proj, position);
    return position;
}
