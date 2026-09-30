#pragma once
#include <engine_types/assets/model/material/material.h>
#include <glm/glm.hpp>

namespace enishi::types {
    struct alignas(16) UniformMaterial {
        static constexpr char UNIFORM_NAME[] = "Material"; // シェーダ側の名前と一致させる必要がある
        glm::vec4 diffuse;
        glm::vec3 specular;
        glm::vec1 shininess;
        glm::vec3 ambient;
        glm::vec1 sphere_mul; // ifによる分岐を減らすため ０-1 で計算
        glm::vec1 sphere_add;
        glm::vec1 edge_flag; // 0-1
        glm::vec2 _pad;
        glm::vec4 base_color_texture_factor{1};
        glm::vec4 base_color_texture_add{};
        glm::vec4 environment_texture_factor{1};
        glm::vec4 environment_texture_add{};
        glm::vec4 shading_ramp_texture_factor{1};
        glm::vec4 shading_ramp_texture_add{};
        glm::vec4 outline_color{};
        glm::vec4 outline_parameters{}; // x: width, remaining components reserved.
    };
    static_assert(sizeof(UniformMaterial) == 192);
    [[nodiscard]] UniformMaterial make_uniform_material(const Material& material);
} // namespace enishi::types
