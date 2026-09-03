#pragma once
#include "interface_native_rigid_body.h"
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <engine_types/physics/joint/physics_joint.h>
#include <engine_types/physics/rigid_body/physics_rigid_body.h>
#include <foundation/result/result.h>
#include <glm/glm.hpp>
#include <memory>
#include <physics/bullet3/motion_state/interface_mmd_motion_state.h>
#include <physics/errors/errors.h>
#include <platform/asset/bone/interface_bone_accessor.h>
#include <platform/asset/bone/interface_bone_list_accessor.h>
#include <platform/physics/rigid_body/interface_rigid_body.h>

namespace enishi::physics::bullet3 {
    class BulletRigidBody : public INativeRigidBody, public platform::IRigidBody {
      private:
        using MotionState = std::unique_ptr<IMMDMotionState>;

        types::PhysicsRigidBody rigid_body_description;
        std::shared_ptr<platform::IBoneAccessor> bone_node;
        MotionState active_motion_state;
        MotionState kinematic_motion_state;

      public:
        explicit BulletRigidBody(void);

        void set_active(const bool active_flag) override;
        void reset(void) override;
        void reset_transform(void) override;
        void apply_local_transform(void) override;
        void apply_global_transform(void) override;

        const types::PhysicsRigidBody& get_rigid_body(void) const noexcept;

      public:
        const btRigidBody* get_native_rigid_body(void) const noexcept override;
        btRigidBody* get_native_rigid_body(void) noexcept override;
    };
} // namespace enishi::physics::bullet3