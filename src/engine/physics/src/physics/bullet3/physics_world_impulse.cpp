#include "physics_world.h"
#include "rigid_body/rigid_body_impulse.h"
#include <cmath>
#include <debug/log/logger.h>
#include <format>

namespace enishi::physics::bullet3 {
    foundation::Result<void, sub_system::PhysicsError> PhysicsWorld::apply_impulse(
        const types::PhysicsHandle& handle, const types::RigidBodyImpulse& impulse) {
        const auto finite = [](glm::vec3 value) {
            return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
        };
        if (!finite(impulse.world_velocity) || !finite(impulse.world_torque) ||
            !finite(impulse.local_velocity) || !finite(impulse.local_torque) ||
            (impulse.world_position.has_value() && !finite(*impulse.world_position))) {
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
        auto& native_body = *body.unwrap();
        if (impulse.world_position.has_value()) {
            if (!native_body.isInWorld() || native_body.isStaticOrKinematicObject()) {
                return foundation::Error(sub_system::PhysicsError::MakeError,
                    "Click impulse rejected: body is disabled, static or kinematic");
            }
            const auto velocity = native_body.getLinearVelocity();
            const auto angular_velocity = native_body.getAngularVelocity();
            apply_rigid_body_impulse(native_body, impulse);
            const auto delta_velocity = native_body.getLinearVelocity() - velocity;
            const auto delta_angular_velocity = native_body.getAngularVelocity() - angular_velocity;
            debug::Logger::info(
                std::format("Bullet click impulse applied: delta velocity=({}, {}, {}), delta "
                            "angular velocity=({}, {}, {})",
                    delta_velocity.x(),
                    delta_velocity.y(),
                    delta_velocity.z(),
                    delta_angular_velocity.x(),
                    delta_angular_velocity.y(),
                    delta_angular_velocity.z()));
        } else if (native_body.isInWorld()) {
            apply_rigid_body_impulse(native_body, impulse);
        }
        return {};
    }
} // namespace enishi::physics::bullet3
