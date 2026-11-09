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
        glm::mat4 converted_transform{1.0f};
        constexpr glm::length_t SPATIAL_DIMENSIONS = 3;
        const auto& basis = transform.getBasis();
        const auto& origin = transform.getOrigin();
        // M[column][row] = basis[row][column], M[3][row] = origin[row].
        // Bullet's SIMD getOpenGLMatrix requires aligned output; GLM matrices may be embedded
        // at scalar alignment, so copy components instead of passing their storage to Bullet.
        for (glm::length_t row = 0; row < SPATIAL_DIMENSIONS; ++row) {
            for (glm::length_t column = 0; column < SPATIAL_DIMENSIONS; ++column) {
                converted_transform[column][row] = static_cast<float>(basis[row][column]);
            }
            converted_transform[SPATIAL_DIMENSIONS][row] = static_cast<float>(origin[row]);
        }
        return converted_transform;
    }
} // namespace enishi::physics::bullet3