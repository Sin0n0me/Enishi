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

namespace enishi::physics::bullet3 {
    class BulletRigidBody : public INativeRigidBody {
      private:
        using MotionState = std::unique_ptr<IMMDMotionState>;

        types::PhysicsRigidBody rigid_body_description;
        std::shared_ptr<platform::IBoneAccessor> bone_node;
        std::unique_ptr<btCollisionShape> shape; // btRigidBodyがポインタを保持し続けるため
        MotionState active_motion_state;
        MotionState kinematic_motion_state;
        std::unique_ptr<btRigidBody> rigid_body;

      public:
        explicit BulletRigidBody(void);
        [[nodiscard]] static foundation::Result<std::unique_ptr<BulletRigidBody>, PhysicsError>
        make(types::PhysicsRigidBody&& rb,
            const std::shared_ptr<platform::IBoneAccessor> bone_node,
            const std::shared_ptr<platform::IBoneListAccessor> bone_list);

        [[nodiscard]] static std::unique_ptr<btCollisionShape> make_shape(
            const types::PhysicsRigidBody& rb);

        void set_active(const bool active_flag);
        void reset(btDiscreteDynamicsWorld* const world);
        void reset_transform(void);
        void apply_local_transform(void);
        void apply_global_transform(void);

        const types::PhysicsRigidBody& get_rigid_body(void) const noexcept;

      public:
        const btRigidBody* get_native_rigid_body(void) const noexcept override;
        btRigidBody* get_native_rigid_body(void) noexcept override;

      private:
        [[nodiscard]] static glm::mat4 make_offset(const types::PhysicsRigidBody& rigid_body,
            const std::shared_ptr<platform::IBoneAccessor> node);

        [[nodiscard]] static std::tuple<MotionState, MotionState> make_motion_state(
            const types::PhysicsRigidBody& rigid_body,
            const std::shared_ptr<platform::IBoneAccessor> node,
            const std::shared_ptr<platform::IBoneListAccessor> bone_list);
    };
} // namespace enishi::physics::bullet3