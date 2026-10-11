#include "physics_world.h"

namespace enishi::physics::bullet3 {
    foundation::Result<void, sub_system::PhysicsError> PhysicsWorld::set_body_enabled(
        const types::PhysicsHandle& handle, const bool enabled) {
        const auto mapped = this->handle_mapper->get(handle);
        if (handle.type != types::PhysicsHandleType::RigidBody || mapped.is_none()) {
            return foundation::Error(sub_system::PhysicsError::MakeError,
                "Visibility target is not a registered rigid body");
        }
        const auto accessor = this->resource_pool->get_native_rigid_body_accessor();
        const auto body = accessor->get_native_rigid_body(mapped.unwrap().resource);
        const auto wrapper = accessor->get_rigid_body(mapped.unwrap().resource);
        if (body.is_none() || body.unwrap() == nullptr || wrapper.is_none()) {
            return foundation::Error(
                sub_system::PhysicsError::MakeError, "Visibility target body is missing");
        }
        auto* const native = body.unwrap().get();
        const auto disabled = this->disabled_body_filters.find(handle);
        if (enabled == (disabled == this->disabled_body_filters.end())) {
            return {};
        }
        const auto joints = this->resource_pool->get_native_joint_accessor()->get_native_joints();
        if (!enabled) {
            const auto* const proxy = native->getBroadphaseProxy();
            if (proxy == nullptr) {
                return foundation::Error(sub_system::PhysicsError::MakeError,
                    "Visibility target is outside the physics world");
            }
            this->disabled_body_filters.emplace(
                handle, std::pair{proxy->m_collisionFilterGroup, proxy->m_collisionFilterMask});
            // Detach constraints before either endpoint leaves the world.
            for (const auto& joint : joints) {
                if ((&joint->getRigidBodyA() == native || &joint->getRigidBodyB() == native) &&
                    joint->getRigidBodyA().isInWorld() && joint->getRigidBodyB().isInWorld()) {
                    this->world->removeConstraint(joint.get());
                }
            }
            this->world->removeRigidBody(native);
            return {};
        }

        // Animation can advance while hidden. Resume at its current pose without stale velocity.
        wrapper.unwrap()->reset_transform();
        wrapper.unwrap()->reset();
        this->world->addRigidBody(native, disabled->second.first, disabled->second.second);
        this->disabled_body_filters.erase(disabled);
        native->activate(true);
        for (const auto& joint : joints) {
            if ((&joint->getRigidBodyA() == native || &joint->getRigidBodyB() == native) &&
                joint->getRigidBodyA().isInWorld() && joint->getRigidBodyB().isInWorld()) {
                this->world->addConstraint(joint.get());
            }
        }
        return {};
    }
} // namespace enishi::physics::bullet3
