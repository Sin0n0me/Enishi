#pragma once
#include <glm/glm.hpp>

namespace enishi::types {
    constexpr float EXTENT_MARGIN = 1.1f;

    struct OBB {
        glm::vec3 center;      // 呼び出し側で選んだ座標空間
        glm::vec3 axis[3];     // 同じ空間の正規直交軸
        glm::vec3 half_extent; // 半径
    };
} // namespace enishi::types
