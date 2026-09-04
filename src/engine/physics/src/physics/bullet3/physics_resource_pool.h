#pragma once
#include "interface_native_physics_accessor.h"
#include "joint/joint_pool.h"
#include "motion_state/motion_state_pool.h"
#include "rigid_body/rigid_body_pool.h"
#include "shape/shape_pool.h"
#include <engine_types/handle/handle_mapper.h>
#include <memory>

namespace enishi::physics::bullet3 {
    class PhysicsResourcePool : public INativePhysicsAccessor {
      private:
        struct PhysicsResourceHandle {
            std::size_t index;
        };

      private:
        types::ResourceMapper<PhysicsResourceHandle> handle_mapper;
        std::unique_ptr<RigidBodyPool> rigid_bodies;
        std::unique_ptr<MotionStatePool> motion_states;
        std::unique_ptr<ShapePool> shapes;
        std::unique_ptr<JointPool> joints;

      public:
        INativeRigidBodyAccessor* get_native_rigid_body_accessor(void) noexcept override;
        const INativeRigidBodyAccessor* get_native_rigid_body_accessor(
            void) const noexcept override;
        INativeMotionStateAccessor* get_native_motion_state_accessor(void) noexcept override;
        const INativeMotionStateAccessor* get_native_motion_state_accessor(
            void) const noexcept override;
        INativeShapeAccessor* get_native_shape_accessor(void) noexcept override;
        const INativeShapeAccessor* get_native_shape_accessor(void) const noexcept override;
        INativeJointAccessor* get_native_joint_accessor(void) noexcept override;
        const INativeJointAccessor* get_native_joint_accessor(void) const noexcept override;
    };
} // namespace enishi::physics::bullet3