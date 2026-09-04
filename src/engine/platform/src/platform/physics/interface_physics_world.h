#pragma once
#include <engine_types/handle/physics/physics_handle.h>
#include <engine_types/physics/joint/physics_joint.h>
#include <engine_types/physics/rigid_body/physics_rigid_body.h>
#include <engine_types/system/delta_time.h>
#include <foundation/result/result.h>
#include <glm/glm.hpp>
#include <platform/animation/interface_bone_updater.h>
#include <platform/errors/physics_errors.h>

namespace enishi::platform {
    class IPhysicsWorld {
      private:
      public:
        virtual ~IPhysicsWorld(void) = default;

        virtual void simulation(const types::DeltaTime& dt) = 0;

        virtual void set_gravity(const glm::vec3& vec) = 0;

        virtual void reset_physics(IBoneUpdater* const updater) = 0;

        virtual void apply_physics(IBoneUpdater* const updater) = 0;

        [[nodiscard]] virtual foundation::Result<types::PhysicsHandle, PhysicsError> add_object(
            void) noexcept = 0;

        [[nodiscard]] virtual foundation::Result<types::PhysicsHandle, PhysicsError> add_rigid_body(
            const types::PhysicsHandle& object_handle,
            types::PhysicsRigidBody&& rigid_body) noexcept = 0;

        [[nodiscard]] virtual foundation::Result<types::PhysicsHandle, PhysicsError> add_joint(
            const types::PhysicsHandle& object_handle, types::PhysicsJoint&& joint) noexcept = 0;
    };
} // namespace enishi::platform