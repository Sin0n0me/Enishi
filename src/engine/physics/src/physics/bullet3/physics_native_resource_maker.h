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
      public:
        using Shape = std::unique_ptr<btCollisionShape>;
        using RigidBody = std::unique_ptr<btRigidBody>;
        using Joint = std::unique_ptr<btGeneric6DofSpringConstraint>;
        using MotionState = std::unique_ptr<IMMDMotionState>;

      public:
        [[nodiscard]] static foundation::Result<Shape, PhysicsError> make_shape(
            const types::PhysicsRigidBody& rb);

        [[nodiscard]] static std::tuple<MotionState, MotionState> make_motion_state(
            const types::PhysicsRigidBody& rigid_body, const bool has_bone);

        [[nodiscard]] static foundation::Result<RigidBody, PhysicsError> make_rigid_body(
            types::PhysicsRigidBody&& rigid_body,
            btCollisionShape* const shape,
            IMMDMotionState* const active_motion_state,
            IMMDMotionState* const kinematic_motion_state);

        [[nodiscard]] static foundation::Result<Joint, PhysicsError> make_joint(
            const types::PhysicsJoint& joint,
            btRigidBody* const rigid_body_a,
            btRigidBody* const rigid_body_b);

        [[nodiscard]] static glm::mat4 make_offset(const types::PhysicsRigidBody& rigid_body);

      private:
        [[nodiscard]] static foundation::VoidResult<PhysicsError> set_joint(
            btGeneric6DofSpringConstraint* const native_joint, const types::PhysicsJoint& joint);
    };
} // namespace enishi::physics::bullet3