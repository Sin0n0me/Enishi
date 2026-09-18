#pragma once
#include "object/physics_object_manager.h"
#include "physics_handle_mapper.h"
#include "physics_resource_pool.h"
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <engine_types/system/delta_time.h>
#include <memory>
#include <sub_system/physics/interface_physics_world.h>
#include <sub_system/physics/interface_physics_world_config_writer.h>

namespace enishi::physics::bullet3 {
    class PhysicsWorld : public sub_system::IPhysicsWorld {
      private:
        std::shared_ptr<sub_system::IPhysicsWorldConfigWriter> config;
        std::unique_ptr<btDbvtBroadphase> broadphase;
        std::unique_ptr<btCollisionDispatcher> dispatcher;
        std::unique_ptr<btSequentialImpulseConstraintSolver> solver;
        std::unique_ptr<btDefaultCollisionConfiguration> collision_config;
        std::unique_ptr<btOverlapFilterCallback> filter_callback;
        std::unique_ptr<btDiscreteDynamicsWorld> world;
        std::unique_ptr<PhysicsObjectManager> object_manager;
        std::shared_ptr<PhysicsResourcePool> resource_pool;
        std::unique_ptr<PhysicsHandleMapper> handle_mapper;

      public:
        explicit PhysicsWorld(std::shared_ptr<sub_system::IPhysicsWorldConfigWriter> config);

        foundation::VoidResult<sub_system::PhysicsError> init(void) override;

        void simulation(const types::DeltaTime& dt) override;
        void set_gravity(const glm::vec3& vec) override;

        foundation::Result<types::PhysicsHandle, sub_system::PhysicsError> add_object(
            void) noexcept override;
        foundation::Result<types::PhysicsHandle, sub_system::PhysicsError> add_rigid_body(
            const types::PhysicsHandle& object_handle,
            const types::PhysicsRigidBody& rigid_body,
            std::shared_ptr<sub_system::IPhysicsBoneViewList> view_list,
            std::shared_ptr<sub_system::IBoneUpdater> updater,
            std::shared_ptr<sub_system::IPhysicsBoneView> physics_bone_view) noexcept override;
        foundation::Result<types::PhysicsHandle, sub_system::PhysicsError> add_joint(
            const types::PhysicsHandle& object_handle,
            const types::PhysicsJoint& joint) noexcept override;

        void reset_physics(sub_system::IBoneUpdater* const updater) override;
        void apply_physics(void) override;

        sub_system::IPhysicsWorldConfigWriter* get_config_writer(void) noexcept override;
        const sub_system::IPhysicsWorldConfigReader* get_config_reader(void) const noexcept override;
    };
} // namespace enishi::physics::bullet3
