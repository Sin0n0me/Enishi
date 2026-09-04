#include "physics_resource_pool.h"

namespace enishi::physics::bullet3 {
    INativeRigidBodyAccessor* PhysicsResourcePool::get_native_rigid_body_accessor(void) noexcept {
        return this->rigid_bodies.get();
    }
    const INativeRigidBodyAccessor* PhysicsResourcePool::get_native_rigid_body_accessor(
        void) const noexcept {
        return this->rigid_bodies.get();
    }
    INativeMotionStateAccessor* PhysicsResourcePool::get_native_motion_state_accessor(
        void) noexcept {
        return this->motion_states.get();
    }
    const INativeMotionStateAccessor* PhysicsResourcePool::get_native_motion_state_accessor(
        void) const noexcept {
        return this->motion_states.get();
    }
    INativeShapeAccessor* PhysicsResourcePool::get_native_shape_accessor(void) noexcept {
        return this->shapes.get();
    }
    const INativeShapeAccessor* PhysicsResourcePool::get_native_shape_accessor(
        void) const noexcept {
        return this->shapes.get();
    }
    INativeJointAccessor* PhysicsResourcePool::get_native_joint_accessor(void) noexcept {
        return this->joints.get();
    }
    const INativeJointAccessor* PhysicsResourcePool::get_native_joint_accessor(
        void) const noexcept {
        return this->joints.get();
    }
} // namespace enishi::physics::bullet3