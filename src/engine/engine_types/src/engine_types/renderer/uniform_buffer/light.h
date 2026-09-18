#pragma once
#include <glm/glm.hpp>

namespace enishi::types {
    struct alignas(16) UniformLight {
        static constexpr char UNIFORM_NAME[] = "Light"; // シェーダ側の名前と一致させる必要がある
        glm::vec4 light_color;
        glm::vec4 light_direction;
    };
} // namespace enishi::types