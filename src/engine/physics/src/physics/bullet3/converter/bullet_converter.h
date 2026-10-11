#pragma once
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

namespace enishi::physics::bullet3 {
    class BulletConverter {
      public:
        static btTransform matrix_to_transform(const glm::mat4& transform) noexcept;
        static glm::mat4 transform_to_matrix(const btTransform& transform) noexcept;
    };
} // namespace enishi::physics::bullet3