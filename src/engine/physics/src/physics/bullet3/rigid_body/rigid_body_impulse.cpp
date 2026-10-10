#include "rigid_body_impulse.h"

namespace enishi::physics::bullet3 {
    namespace {
        btVector3 linear_vector(glm::vec3 value) {
            return {value.x, value.y, -value.z};
        }

        btVector3 angular_vector(glm::vec3 value) {
            // Torque is an axial vector: reflecting Z also changes its handedness.
            return {-value.x, -value.y, value.z};
        }
    } // namespace

    void apply_rigid_body_impulse(btRigidBody& body, const types::RigidBodyImpulse& impulse) {
        if (impulse.reset_velocity) {
            const btVector3 zero(0, 0, 0);
            body.setLinearVelocity(zero);
            body.setAngularVelocity(zero);
            body.setInterpolationLinearVelocity(zero);
            body.setInterpolationAngularVelocity(zero);
            body.setInterpolationWorldTransform(body.getWorldTransform());
            body.clearForces();
        }
        const auto& basis = body.getWorldTransform().getBasis();
        const auto linear_impulse =
            linear_vector(impulse.world_velocity) + basis * linear_vector(impulse.local_velocity);
        if (impulse.world_position.has_value()) {
            // r = hit - center of mass; Bullet applies delta-v = J/m and delta-L = r cross J.
            const auto offset = linear_vector(*impulse.world_position) - body.getCenterOfMassPosition();
            body.applyImpulse(linear_impulse, offset);
        } else {
            body.applyCentralImpulse(linear_impulse);
        }
        body.applyTorqueImpulse(
            angular_vector(impulse.world_torque) + basis * angular_vector(impulse.local_torque));
        body.activate(true);
    }
} // namespace enishi::physics::bullet3
