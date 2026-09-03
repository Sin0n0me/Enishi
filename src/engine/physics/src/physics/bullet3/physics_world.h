#pragma once
#include "object/physics_object_manager.h"
#include "physics_handle_mapper.h"
#include "physics_resource_pool.h"
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
        std::unique_ptr<btOverlapFilterCallback> filter_callback;
        std::unique_ptr<btDiscreteDynamicsWorld> world;
        std::unique_ptr<PhysicsObjectManager> object_maanger;
        std::unique_ptr<PhysicsResourcePool> resource_pool;
        std::unique_ptr<PhysicsHandleMapper> handle_mapper;

      public:
        explicit PhysicsWorld(void);

        static foundation::Result<std::unique_ptr<PhysicsWorld>, PhysicsError> make(void);

        void simulation(const types::DeltaTime& dt) override;
        void set_gravity(const glm::vec3& vec) override;

        types::PhysicsHandle add_rigid_body(types::PhysicsRigidBody&& rigid_body) override;
        types::PhysicsHandle add_joint(types::PhysicsJoint&& joint) override;

        void reset_physics(void) override;
        void apply_physics(void) override;
    };
} // namespace enishi::physics::bullet3