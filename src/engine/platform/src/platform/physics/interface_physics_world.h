#pragma once
#include <engine_types/handle/physics/physics_handle.h>
#include <engine_types/physics/joint/physics_joint.h>
#include <engine_types/physics/rigid_body/physics_rigid_body.h>
#include <engine_types/system/delta_time.h>
#include <foundation/result/result.h>
#include <glm/glm.hpp>
#include <memory>
#include <platform/animation/interface_bone_updater.h>
#include <platform/bone_ststem/interface_bone_view.h>
#include <platform/errors/physics_errors.h>
#include <platform/physics/bone/interface_physics_bone_view.h>
#include <platform/physics/interface_physics_world_config_writer.h>
#include <platform/physics/rigid_body/interface_rigid_body.h>

namespace enishi::platform {
    class IPhysicsWorld {
      private:
      public:
        virtual ~IPhysicsWorld(void) = default;

        [[nodiscard]] virtual foundation::VoidResult<PhysicsError> init(void) = 0;

        virtual void simulation(const types::DeltaTime& dt) = 0;

        virtual void set_gravity(const glm::vec3& vec) = 0;

        virtual void reset_physics(IBoneUpdater* const updater) = 0;

        virtual void apply_physics(IBoneUpdater* const updater) = 0;

        [[nodiscard]] virtual IPhysicsWorldConfigWriter* get_config_writer(void) noexcept = 0;

        [[nodiscard]] virtual const IPhysicsWorldConfigReader* get_config_reader(
            void) const noexcept = 0;

        [[nodiscard]] virtual foundation::Result<types::PhysicsHandle, PhysicsError> add_object(
            void) noexcept = 0;

        [[nodiscard]] virtual foundation::Result<types::PhysicsHandle, PhysicsError> add_rigid_body(
            const types::PhysicsHandle& object_handle,
            types::PhysicsRigidBody&& rigid_body,
            std::shared_ptr<platform::IBoneView> bone_view,
            std::shared_ptr<platform::IBoneUpdater> updater,
            std::shared_ptr<platform::IPhysicsBoneView> physics_bone_view) noexcept = 0;

        [[nodiscard]] virtual foundation::Result<types::PhysicsHandle, PhysicsError> add_joint(
            const types::PhysicsHandle& object_handle, types::PhysicsJoint&& joint) noexcept = 0;
    };
} // namespace enishi::platform