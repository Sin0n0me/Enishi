#pragma once
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <engine_types/handle/physics/physics_handle.h>
#include <engine_types/physics/joint/physics_joint.h>
#include <engine_types/physics/rigid_body/physics_rigid_body.h>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <memory>
#include <physics/bullet3/motion_state/interface_mmd_motion_state.h>
#include <physics/errors/errors.h>
#include <platform/physics/interface_physics_handle_mapper.h>

namespace enishi::physics::bullet3 {
    class PhysicsNativeResourceMaker {
      private:
        using MotionState = std::unique_ptr<IMMDMotionState>;

      public:
        [[nodiscard]] static foundation::Result<std::unique_ptr<btRigidBody>, PhysicsError>
        set_rigid_body(btRigidBody* const native_rigid_body, types::PhysicsRigidBody&& rigid_body);

        [[nodiscard]] static glm::mat4 make_offset(const types::PhysicsRigidBody& rigid_body);

      private:
        [[nodiscard]] static std::unique_ptr<btCollisionShape> make_shape(
            const types::PhysicsRigidBody& rb);

        [[nodiscard]] static std::tuple<MotionState, MotionState> make_motion_state(
            const types::PhysicsRigidBody& rigid_body);
    };
} // namespace enishi::physics::bullet3