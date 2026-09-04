#pragma once
#include <btBulletDynamicsCommon.h>
#include <engine_types/physics/joint/physics_joint.h>
#include <engine_types/physics/rigid_body/physics_rigid_body.h>
#include <glm/glm.hpp>
#include <memory>
#include <physics/bullet3/rigid_body/rigid_body.h>
#include <platform/animation/interface_bone_updater.h>

namespace enishi::physics::bullet3 {
    class BulletJoint {
      private:
        std::unique_ptr<btTypedConstraint> constraint;

      public:
        explicit BulletJoint(const types::PhysicsJoint& joint,
            BulletRigidBody& rigid_body_a,
            BulletRigidBody& rigid_body_b);

        btTypedConstraint* get_constraint(void) noexcept;
        const btTypedConstraint* get_constraint(void) const noexcept;
    };
} // namespace enishi::physics::bullet3