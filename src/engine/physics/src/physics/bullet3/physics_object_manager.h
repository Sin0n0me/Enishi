#pragma once
#include <engine_types/handle/handle_allocator.h>
#include <engine_types/handle/handle_mapper.h>
#include <engine_types/handle/physics/physics_handle.h>
#include <foundation/result/result.h>
#include <memory>
#include <physics/errors/errors.h>
#include <platform/physics/interface_physics_paramter_readable.h>
#include <platform/physics/interface_physics_world.h>
#include <unordered_map>

namespace enishi::physics::bullet3 {
    class PhysicsObjectManager {
      private:
      public:
        types::PhysicsHandle make_object(void);

        foundation::Result<types::PhysicsHandle, PhysicsError> add_rigid_body(
            const types::PhysicsHandle& object_handle, types::PhysicsRigidBody&& rigid_body);

        foundation::Result<types::PhysicsHandle, PhysicsError> add_joint(
            const types::PhysicsHandle& object_handle, types::PhysicsJoint&& joint);
    };
} // namespace enishi::physics::bullet3