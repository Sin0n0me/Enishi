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
#include <physics/common/physics_bone_views.h>
#include <physics/errors/errors.h>

namespace enishi::physics::bullet3 {
    class BulletRigidBody : public sub_system::IRigidBody {
      private:
        // Borrow heap objects owned by the same resource pool as this wrapper.
        btRigidBody& rigid_body;
        IMMDMotionState& active_motion_state;
        IMMDMotionState& kinematic_motion_state;
        PhysicsBoneViews views;
        types::RigidBodyKind kind;
        types::BoneIndex bone_index;
        glm::mat4 animation_transform{1};
        bool has_animation_transform{false};

      public:
        explicit BulletRigidBody(btRigidBody& rigid_body,
            IMMDMotionState& active_motion_state,
            IMMDMotionState& kinematic_motion_state,
            PhysicsBoneViews&& views,
            types::RigidBodyKind kind,
            types::BoneIndex bone_index);

        void set_active(const bool active_flag) override;
        void reset(void) override;
        void reset_transform(void) override;
        void sync_animation(void) override;
        void apply_local_transform(void) override;
        void apply_global_transform(void) override;

      private:
        bool is_in_world(void) const;
    };
} // namespace enishi::physics::bullet3
