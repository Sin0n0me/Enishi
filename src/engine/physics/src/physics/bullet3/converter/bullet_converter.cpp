#include "bullet_converter.h"
#include <glm/gtc/type_ptr.hpp>
#include <physics/helper/helper.h>

namespace enishi::physics::bullet3 {
    btTransform BulletConverter::matrix_to_transform(const glm::mat4& transform) noexcept {
        btTransform converted_transform;
        converted_transform.setFromOpenGLMatrix(glm::value_ptr(transform));
        return converted_transform;
    }

    glm::mat4 BulletConverter::transform_to_matrix(const btTransform& transform) noexcept {
        glm::mat4 converted_transform{1.0};
        transform.getOpenGLMatrix(glm::value_ptr(converted_transform));
        return converted_transform;
    }
} // namespace enishi::physics::bullet3