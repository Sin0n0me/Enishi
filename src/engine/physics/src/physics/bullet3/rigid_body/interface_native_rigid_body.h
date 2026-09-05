#pragma once
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

namespace enishi::physics::bullet3 {
    class INativeRigidBody {
      public:
        virtual ~INativeRigidBody(void) noexcept = default;

        [[nodiscard]] virtual const btRigidBody* get_native_rigid_body(void) const noexcept = 0;
        [[nodiscard]] virtual btRigidBody* get_native_rigid_body(void) noexcept = 0;
    };
} // namespace enishi::physics::bullet3
