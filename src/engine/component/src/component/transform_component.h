#pragma once
#include <glm/glm.hpp>

namespace enishi::component {
    struct TransformComponent {
        glm::mat4 translate;
        glm::mat4 rotate;
        glm::mat4 scale;
    };
} // namespace enishi::component
