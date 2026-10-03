#pragma once
#include <glm/glm.hpp>

namespace enishi::types {
    struct alignas(16) UniformCamera {
        static constexpr char UNIFORM_NAME[] = "Camera"; // シェーダ側の名前と一致させる必要がある
        glm::mat4 world;
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 mvp;
    };
} // namespace enishi::types