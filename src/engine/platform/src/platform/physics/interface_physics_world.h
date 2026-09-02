#pragma once
#include <engine_types/handle/physics/physics_handle.h>
#include <engine_types/physics/joint/physics_joint.h>
#include <engine_types/physics/rigid_body/physics_rigid_body.h>
#include <engine_types/system/delta_time.h>
#include <glm/glm.hpp>

namespace enishi::platform {
    class IPhysicsWorld {
      private:
      public:
        virtual ~IPhysicsWorld(void) = default;

        virtual void simulation(const types::DeltaTime& dt) = 0;

        virtual void set_gravity(const glm::vec3& vec) = 0;

        [[nodiscard]] virtual types::PhysicsHandle add_rigid_body(
            types::PhysicsRigidBody&& rigid_body) = 0;

        [[nodiscard]] virtual types::PhysicsHandle add_joint(types::PhysicsJoint&& joint) = 0;

        //[[nodiscard]] virtual void add_object(void) = 0;
    };
} // namespace enishi::platform