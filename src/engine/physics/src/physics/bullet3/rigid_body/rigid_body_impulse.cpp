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
        body.applyCentralImpulse(
            linear_vector(impulse.world_velocity) + basis * linear_vector(impulse.local_velocity));
        body.applyTorqueImpulse(
            angular_vector(impulse.world_torque) + basis * angular_vector(impulse.local_torque));
        body.activate(true);
    }
} // namespace enishi::physics::bullet3
