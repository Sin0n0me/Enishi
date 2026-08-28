#pragma once
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

      public:
        void simulation(const types::DeltaTime& dt) override;

      private:
    };
} // namespace enishi::physics::bullet3