#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace enishi::physics::bullet3 {
    glm::mat4 inverse_z(const glm::mat4& matrix) {
        return matrix * glm::scale(glm::mat4{1.0f}, glm::vec3(1.0f, 1.0f, -1.0f)) * matrix;
    }
} // namespace enishi::physics::bullet3
