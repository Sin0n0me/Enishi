#include "physics_world.h"
#include "filter/filter_callback.h"
#include "joint/joint.h"
#include "physics_native_resource_maker.h"
#include "rigid_body/rigid_body.h"

namespace enishi::physics::bullet3 {
    constexpr float GRAVITY = 9.80665f;

    PhysicsWorld::PhysicsWorld(void)
        : resource_pool(std::make_unique<PhysicsResourcePool>())
        , object_maanger(std::make_unique<PhysicsObjectManager>()) {
    }

    foundation::Result<std::unique_ptr<PhysicsWorld>, PhysicsError> PhysicsWorld::make(void) {
        auto physics_world = std::make_unique<PhysicsWorld>();
        auto& world = physics_world->world;
        auto& resource = physics_world->resource_pool;

        auto& info = world->getSolverInfo();
        info.m_numIterations = 10;
        info.m_solverMode = btSolverMode::SOLVER_SIMD;

        // 重力設定
        world->setGravity(btVector3(0, -GRAVITY * 2.0f, 0));

        // 地面作成
        auto [motion_state_index, motion_state] =
            resource->get_native_motion_state_accessor()->emplace_native_motion_state(
                std::make_unique<btDefaultMotionState>(btTransform::getIdentity()));
        auto [shape_index, shape] = resource->get_native_shape_accessor()->emplace_native_shape(
            std::make_unique<btStaticPlaneShape>(btVector3(0, 1, 0), 0.0f));
        auto [rigid_body_index, rigid_body] =
            resource->get_native_rigid_body_accessor()->emplace_native_rigid_body(
                std::make_unique<btRigidBody>(btRigidBody::btRigidBodyConstructionInfo{
                    0,
                    motion_state.get(),
                    shape.get(),
                    btVector3(0, 0, 0),
                }));
        world->addRigidBody(rigid_body.get());

        {
            auto filter_callback = std::make_unique<FilterCallback>();
            filter_callback->add_proxy(rigid_body->getBroadphaseProxy());
            physics_world->filter_callback = std::move(filter_callback);
            world->getPairCache()->setOverlapFilterCallback(physics_world->filter_callback.get());
        }

        return physics_world;
    }
    void PhysicsWorld::simulation(const types::DeltaTime& dt) {
        if (!this->parameter->can_update()) {
            return;
        }

        const auto step_time = this->parameter->get_step_time();
        const auto max_step = this->parameter->get_max_step_count();

        this->world->stepSimulation(
            static_cast<btScalar>(dt.to_float()), max_step, static_cast<btScalar>(step_time));
    }

    void PhysicsWorld::set_gravity(const glm::vec3& vec) {
        this->world->setGravity(btVector3(vec.x, vec.y, vec.z));
    }

    types::PhysicsHandle PhysicsWorld::add_rigid_body(types::PhysicsRigidBody&& rigid_body) {
        const auto [resource_handle, native_rigid_body] =
            this->resource_pool->get_native_rigid_body_accessor()->make_native_rigid_body();

        const auto mask = rigid_body.group_target;
        const auto group = 1 << rigid_body.group_index;

        auto&& result = PhysicsNativeResourceMaker::set_rigid_body(
            native_rigid_body.get(), std::move(rigid_body));
        if (result.is_err()) {
            return result;
        }

        // Bulletの世界に追加
        this->world->addRigidBody(native_rigid_body.get(), group, mask);

        return this->handle_mapper->make(types::PhysicsHandleType::RigidBody,
            types::ResourceHandles{
                .resource = resource_handle,
            });
    }

    types::PhysicsHandle PhysicsWorld::add_joint(types::PhysicsJoint&& joint) {
        const size_t size = this->rigid_bodies.size();
        const size_t rigid_body_a = joint.rigid_body_a;
        const size_t rigid_body_b = joint.rigid_body_b;
        // 0 index
        if (size < (rigid_body_a + 1) || size < (rigid_body_b + 1)) {
            return false;
        }
        if (rigid_body_a == rigid_body_b) {
            return false;
        }

        std::unique_ptr<BulletJoint> mmd_joint = std::make_unique<BulletJoint>(joint,
            this->rigid_bodies.at(rigid_body_a).get(),
            this->rigid_bodies.at(rigid_body_b).get());

        const auto bt_joint = mmd_joint.get();
        this->world->addConstraint(bt_joint->get_constraint());

        this->joints.emplace_back(std::move(mmd_joint));

        return types::PhysicsHandle();
    }

    void PhysicsWorld::reset_physics(void) {
        const auto rigid_bodies =
            this->resource_pool->get_native_rigid_body_accessor()->get_native_rigid_bodies();

        this->object_maanger->make_object();

        for (const auto& rigid_body : rigid_bodies) {
        }

        for (auto& rb : this->rigid_bodies) {
            rb->set_active(false);
            rb->reset_transform();
        }

        for (auto& rb : this->rigid_bodies) {
            rb->apply_global_transform();
        }
        for (auto& rb : this->rigid_bodies) {
            rb->apply_local_transform();
        }
        for (auto& node : this->root_nodes) {
            node->update_global();
        }
        for (auto& rb : this->rigid_bodies) {
            rb->reset(world);
        }
    }

    void PhysicsWorld::apply_physics(void) {
    }
} // namespace enishi::physics::bullet3