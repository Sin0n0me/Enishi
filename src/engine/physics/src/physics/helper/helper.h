#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace enishi::physics {
    glm::mat4 inverse_z(const glm::mat4& matrix);
    glm::mat4 inverse_z(glm::mat4&& matrix);
} // namespace enishi::physics
