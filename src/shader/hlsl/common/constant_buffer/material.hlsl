cbuffer Material {
    float4 diffuse;
    float3 specular;
    float shininess;
    float3 ambient;
    float sphere_mul; // ifによる分岐を減らすため ０-1 で計算
    float sphere_add;
    float edge_flag; // 0-1
    float2 _pad;
};

float4 apply_lighting(
    const float4 color,
    const float3 light_color,
    const float3 light_direction,
    const float3 normal
){    
    const float3 normalized_normal = normalize(normal);
    const float3 normalized_light = normalize(light_direction);
  
    const float dot_nl = saturate(dot(normalized_normal, normalized_light));
    const float4 fixed_diffuse = diffuse * dot_nl;
   
    const float3 lighting_color = light_color.rgb * fixed_diffuse.rgb + ambient;

    return float4(color.rgb * saturate(lighting_color), color.a);
}

// Specular(Blinn-Phong)
float4 apply_specular(
    const float4 color,
    const float3 position,
    const float3 light_direction,
    const float3 normal
) {
    const float3 normalized_normal = normalize(normal);
    const float3 normalized_light = normalize(-light_direction);
    const float3 view_direction = normalize(-position);
    
    // マイナスの場合は裏側をハイライトしてしまう
    // 極僅かなスペキュラも除外する
    const float normal_dot_light = dot(normalized_normal, normalized_light);
    if (normal_dot_light < 1e-6) {
        return color;
    }

    // 長さが限りなく0に近いと後続で計算結果がInfになる可能性があり, チラつきの原因になる
    const float3 half_vec_raw = normalized_light + view_direction;
    const float half_len_sq = dot(half_vec_raw, half_vec_raw);
    if (half_len_sq < 1e-6) {
        return color;
    }
    
    const float3 half_vec = half_vec_raw * rsqrt(half_len_sq);
    const float normal_dot_half = saturate(dot(normalized_normal, half_vec));
    const float spec_factor = normal_dot_half > 0.0 ? pow(normal_dot_half, shininess) : 0.0;
    const float3 fixed_specular = specular * spec_factor;
    
    return float4(color.rgb + fixed_specular, color.a);
}


float2 calc_sphere_uv(
    const float3 position,
    const float3 normal
) {
    const float3 view_dir = normalize(-position);
    const float3 reflect_dir = reflect(-view_dir, normalize(normal));
    const float dot_reflect = dot(reflect_dir.xy, reflect_dir.xy);
    const float pow_reflect = (reflect_dir.z + 1.0f) * (reflect_dir.z + 1.0f);
    const float m = 2.0f * sqrt(dot_reflect + pow_reflect);
    const float inv_m = rcp(max(m, 1e-5f));

    return float2(
        reflect_dir.x * inv_m + 0.5f,
        -reflect_dir.y * inv_m + 0.5f
    );
}

// IFによる分岐を減らすために以下のような回りくどい計算をしている
float4 apply_sphere_map(
    const float4 color,
    const float3 sphere_color
) {
    const float3 base_color = color.rgb;
    const float3 mul_case = (base_color * sphere_color) * sphere_mul;
    const float3 add_case = (base_color + sphere_color) * sphere_add;
    const float3 no_case = base_color * (1.0 - (sphere_mul + sphere_add)); // 適用なしの場合
    const float3 result = no_case + add_case + mul_case;

    return float4(result, color.a);
}
