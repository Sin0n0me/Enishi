#pragma once
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
    };
} // namespace enishi::types