#pragma once
#include <glm/glm.hpp>

namespace enishi::types {
    constexpr float EXTENT_MARGIN = 1.1f;

    struct OBB {
        glm::vec3 center;      // ローカル空間
        glm::vec3 axis[3];     // 正規化された軸（ローカル）
        glm::vec3 half_extent; // 半径
    };
} // namespace enishi::types
