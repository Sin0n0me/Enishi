#pragma once
#include "physics_object_manager.h"
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <engine_types/system/delta_time.h>
#include <memory>
#include <platform/physics/interface_physics_paramter_readable.h>
#include <platform/physics/interface_physics_world.h>

namespace enishi::physics::bullet3 {
    class PhysicsWorld : public platform::IPhysicsWorld {
      private:
        std::shared_ptr<platform::IPhysicsParameterReadable> parameter;
        std::unique_ptr<btDbvtBroadphase> broadphase;
        std::unique_ptr<btCollisionDispatcher> dispatcher;
        std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
        std::unique_ptr<btDefaultCollisionConfiguration> collision_config;
        std::unique_ptr<btDiscreteDynamicsWorld> world;
        std::unique_ptr<PhysicsObjectManager> object_maanger;

      public:
        explicit PhysicsWorld(void);

        static foundation::Result<std::unique_ptr<PhysicsWorld>, PhysicsError> make(void);

        void simulation(const types::DeltaTime& dt) override;
        void set_gravity(const glm::vec3& vec) override;

        types::PhysicsHandle add_rigid_body(types::PhysicsRigidBody&& rigid_body) override;
        types::PhysicsHandle add_joint(types::PhysicsJoint&& joint) override;
    };
} // namespace enishi::physics::bullet3