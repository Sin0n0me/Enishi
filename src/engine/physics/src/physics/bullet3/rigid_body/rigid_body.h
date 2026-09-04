#pragma once
#include "interface_native_rigid_body.h"
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <engine_types/physics/joint/physics_joint.h>
#include <engine_types/physics/rigid_body/physics_rigid_body.h>
#include <foundation/result/result.h>
#include <glm/glm.hpp>
#include <memory>
#include <physics/bullet3/interface_native_physics_accessor.h>
#include <physics/bullet3/motion_state/interface_mmd_motion_state.h>
#include <physics/errors/errors.h>
#include <platform/bone_ststem/interface_bone_view.h>
#include <platform/physics/bone/interface_physics_bone_view.h>
#include <platform/physics/rigid_body/interface_rigid_body.h>

namespace enishi::physics::bullet3 {
    class BulletRigidBody : public platform::IRigidBody {
      private:
        std::shared_ptr<INativePhysicsAccessor> view;
        std::shared_ptr<platform::IBoneView> bone_view;
        std::shared_ptr<platform::IBoneUpdater> updater;
        std::shared_ptr<platform::IPhysicsBoneView> physics_bone_view;
        types::HandleId rigid_body;
        types::HandleId active_motion_state;
        types::HandleId kinematic_motion_state;
        types::RigidBodyKind kind;

      public:
        explicit BulletRigidBody(std::shared_ptr<INativePhysicsAccessor> view,
            std::shared_ptr<platform::IBoneView> bone_view,
            std::shared_ptr<platform::IBoneUpdater> updater,
            std::shared_ptr<platform::IPhysicsBoneView> physics_bone_view);

        void set_active(const bool active_flag) override;
        void reset(void) override;
        void reset_transform(void) override;
        void apply_local_transform(void) override;
        void apply_global_transform(void) override;
    };
} // namespace enishi::physics::bullet3