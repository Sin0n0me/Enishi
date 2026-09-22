#include "helper.h"

namespace enishi::physics {
    glm::mat4 inverse_z(const glm::mat4& matrix) {
        return matrix * glm::scale(glm::mat4{1.0f}, glm::vec3(1.0f, 1.0f, -1.0f)) * matrix;
    }
    glm::mat4 inverse_z(glm::mat4&& matrix) {
        return matrix * glm::scale(glm::mat4{1.0f}, glm::vec3(1.0f, 1.0f, -1.0f)) * matrix;
    }
} // namespace enishi::physics