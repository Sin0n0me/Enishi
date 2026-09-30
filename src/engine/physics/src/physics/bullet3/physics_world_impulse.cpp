#include "physics_world.h"
#include "rigid_body/rigid_body_impulse.h"
#include <cmath>

namespace enishi::physics::bullet3 {
    foundation::Result<void, sub_system::PhysicsError> PhysicsWorld::apply_impulse(
        const types::PhysicsHandle& handle, const types::RigidBodyImpulse& impulse) {
        const auto finite = [](glm::vec3 value) {
            return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
        };
        if (!finite(impulse.world_velocity) || !finite(impulse.world_torque) ||
            !finite(impulse.local_velocity) || !finite(impulse.local_torque)) {
            return foundation::Error(
                sub_system::PhysicsError::MakeError, "Non-finite rigid body impulse");
        }
        const auto mapped = this->handle_mapper->get(handle);
        if (handle.type != types::PhysicsHandleType::RigidBody || mapped.is_none()) {
            return foundation::Error(sub_system::PhysicsError::MakeError,
                "Impulse target is not a registered rigid body");
        }
        const auto body =
            this->resource_pool->get_native_rigid_body_accessor()->get_native_rigid_body(
                mapped.unwrap().resource);
        if (body.is_none() || body.unwrap() == nullptr) {
            return foundation::Error(
                sub_system::PhysicsError::MakeError, "Impulse target body is missing");
        }
        apply_rigid_body_impulse(*body.unwrap(), impulse);
        return {};
    }
} // namespace enishi::physics::bullet3
