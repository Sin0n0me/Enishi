#pragma once
#include "object/physics_object_manager.h"
#include "physics_handle_mapper.h"
#include "physics_resource_pool.h"
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <engine_types/system/delta_time.h>
#include <memory>
#include <platform/physics/interface_physics_world.h>
#include <platform/physics/interface_physics_world_config_writer.h>

namespace enishi::physics::bullet3 {
    class PhysicsWorld : public platform::IPhysicsWorld {
      private:
        std::shared_ptr<platform::IPhysicsWorldConfigWriter> config;
        std::unique_ptr<btDbvtBroadphase> broadphase;
        std::unique_ptr<btCollisionDispatcher> dispatcher;
        std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
        std::unique_ptr<btDefaultCollisionConfiguration> collision_config;
        std::unique_ptr<btOverlapFilterCallback> filter_callback;
        std::unique_ptr<btDiscreteDynamicsWorld> world;
        std::unique_ptr<PhysicsObjectManager> object_maanger;
        std::shared_ptr<PhysicsResourcePool> resource_pool;
        std::unique_ptr<PhysicsHandleMapper> handle_mapper;

      public:
        explicit PhysicsWorld(std::shared_ptr<platform::IPhysicsWorldConfigWriter> config);

        foundation::VoidResult<platform::PhysicsError> init(void) override;

        void simulation(const types::DeltaTime& dt) override;
        void set_gravity(const glm::vec3& vec) override;

        foundation::Result<types::PhysicsHandle, platform::PhysicsError> add_object(
            void) noexcept override;
        foundation::Result<types::PhysicsHandle, platform::PhysicsError> add_rigid_body(
            const types::PhysicsHandle& object_handle,
            types::PhysicsRigidBody&& rigid_body,
            std::shared_ptr<platform::IBoneView> bone_view,
            std::shared_ptr<platform::IBoneUpdater> updater,
            std::shared_ptr<platform::IPhysicsBoneView> physics_bone_view) noexcept override;
        foundation::Result<types::PhysicsHandle, platform::PhysicsError> add_joint(
            const types::PhysicsHandle& object_handle,
            types::PhysicsJoint&& joint) noexcept override;

        void reset_physics(platform::IBoneUpdater* const updater) override;
        void apply_physics(platform::IBoneUpdater* const updater) override;

        platform::IPhysicsWorldConfigWriter* get_config_writer(void) noexcept override;
        const platform::IPhysicsWorldConfigReader* get_config_reader(void) const noexcept override;
    };
} // namespace enishi::physics::bullet3