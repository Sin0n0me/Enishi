#include "joint.h"

namespace enishi::physics::bullet3 {
    BulletJoint::BulletJoint(const types::PhysicsJoint& joint,
        BulletRigidBody& rigid_body_a,
        BulletRigidBody& rigid_body_b) {
        /*
         */
    }

    btTypedConstraint* BulletJoint::get_constraint(void) noexcept {
        return this->constraint.get();
    }

    const btTypedConstraint* BulletJoint::get_constraint(void) const noexcept {
        return this->constraint.get();
    }
} // namespace enishi::physics::bullet3