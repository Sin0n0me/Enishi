#include "helper.h"

namespace enishi::physics {
    glm::mat4 inverse_z(const glm::mat4& matrix) {
        const auto reflection = glm::scale(glm::mat4{1.0f}, glm::vec3(1.0f, 1.0f, -1.0f));
        return reflection * matrix * reflection;
    }
    glm::mat4 inverse_z(glm::mat4&& matrix) {
        return inverse_z(static_cast<const glm::mat4&>(matrix));
    }
} // namespace enishi::physics
