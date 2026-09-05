#include "physics_world.h"
#include "filter/filter_callback.h"
#include "joint/joint.h"
#include "motion_state/default/model_default_motion_state.h"
#include "physics_native_resource_maker.h"
#include "physics_world.h"
#include "rigid_body/rigid_body.h"
#include <physics/common/physics_bone_views.h>

namespace enishi::physics::bullet3 {
    constexpr float GRAVITY = 9.80665f;

    PhysicsWorld::PhysicsWorld(std::shared_ptr<platform::IPhysicsWorldConfigWriter> config)
        : resource_pool(std::make_shared<PhysicsResourcePool>())
        , object_maanger(std::make_unique<PhysicsObjectManager>())
        , broadphase(std::make_unique<btDbvtBroadphase>())
        , collision_config(std::make_unique<btDefaultCollisionConfiguration>())
        , solver(std::make_unique<btSequentialImpulseConstraintSolver>())
        , config(config) {
        // BulletのWorldの作成
        this->dispatcher = std::make_unique<btCollisionDispatcher>(this->collision_config.get());
        this->world = std::make_unique<btDiscreteDynamicsWorld>(this->dispatcher.get(),
            this->broadphase.get(),
            this->solver.get(),
            this->collision_config.get());
    }

    foundation::VoidResult<platform::PhysicsError> enishi::physics::bullet3::PhysicsWorld::init(
        void) {
        auto& info = this->world->getSolverInfo();
        info.m_numIterations = 10;
        info.m_solverMode = btSolverMode::SOLVER_SIMD;

        // 重力設定
        this->world->setGravity(btVector3(0, -GRAVITY * 2.0f, 0));

        // 地面作成
        auto motion_state_view = this->resource_pool->get_native_motion_state_accessor();
        auto ground_motion_state =
            std::make_shared<ModelDefaultMotionState>(btTransform::getIdentity());
        auto [motion_state_index, motion_state] =
            motion_state_view->emplace_native_motion_state(ground_motion_state);
        auto opt = motion_state_view->link_motion_state(
            motion_state_index, std::move(ground_motion_state));
        if (opt.is_none()) {
        }

        auto [shape_index, shape] =
            this->resource_pool->get_native_shape_accessor()->emplace_native_shape(
                std::make_unique<btStaticPlaneShape>(btVector3(0, 1, 0), 0.0f));

        auto [rigid_body_index, rigid_body] =
            this->resource_pool->get_native_rigid_body_accessor()->emplace_native_rigid_body(
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
            this->filter_callback = std::move(filter_callback);
            world->getPairCache()->setOverlapFilterCallback(this->filter_callback.get());
        }

        return {};
    }

    void PhysicsWorld::simulation(const types::DeltaTime& dt) {
        if (!this->config->can_update()) {
            return;
        }

        const auto fixed_step_time = this->config->get_fixed_step_time();
        const auto max_step = this->config->get_max_step_count();

        this->world->stepSimulation(
            static_cast<btScalar>(dt.to_float()), max_step, static_cast<btScalar>(fixed_step_time));
    }

    void PhysicsWorld::set_gravity(const glm::vec3& vec) {
        this->world->setGravity(btVector3(vec.x, vec.y, vec.z));
    }

    foundation::Result<types::PhysicsHandle, platform::PhysicsError> PhysicsWorld::add_object(
        void) noexcept {
        return this->handle_mapper->make(types::PhysicsHandleType::PhysicsObject,
            types::ResourceHandles{
                .resource = this->object_maanger->add_object(),
            });
    }

    foundation::Result<types::PhysicsHandle, platform::PhysicsError> PhysicsWorld::add_rigid_body(
        const types::PhysicsHandle& object_handle,
        types::PhysicsRigidBody&& rigid_body_description,
        std::shared_ptr<platform::IBoneView> bone_view,
        std::shared_ptr<platform::IBoneUpdater> updater,
        std::shared_ptr<platform::IPhysicsBoneView> physics_bone_view) noexcept {
        const auto mask = rigid_body_description.group_mask;
        const auto group = 1 << rigid_body_description.group_index;

        auto&& [kinematic_motion_state, active_motion_state] =
            PhysicsNativeResourceMaker::make_motion_state(rigid_body_description, false);
        auto&& result_shape = PhysicsNativeResourceMaker::make_shape(rigid_body_description);
        if (result_shape.is_err()) {
            return result_shape.propagation(platform::PhysicsError::MakeError);
        }
        auto&& shape = result_shape.unwrap_mut();

        auto&& rigid_body_result =
            PhysicsNativeResourceMaker::make_rigid_body(std::move(rigid_body_description),
                shape.get(),
                active_motion_state.get(),
                kinematic_motion_state.get());
        if (rigid_body_result.is_err()) {
            return rigid_body_result.propagation(platform::PhysicsError::MakeError);
        }
        const auto rigid_body_view = this->resource_pool->get_native_rigid_body_accessor();

        // モーションステートの作成
        const auto motion_accessor = this->resource_pool->get_native_motion_state_accessor();
        const auto [kinematic_motion_state_handle, kms] =
            motion_accessor->emplace_native_motion_state(std::move(kinematic_motion_state));
        const auto [active_motion_state_handle, ams] =
            motion_accessor->emplace_native_motion_state(std::move(active_motion_state));

        // 形状の作成
        const auto [shape_handle, s] =
            this->resource_pool->get_native_shape_accessor()->emplace_native_shape(
                std::move(shape));

        // 剛体の作成
        const auto [rigid_body_handle, native_rigid_body] =
            rigid_body_view->emplace_native_rigid_body(std::move(rigid_body_result).unwrap_mut());
        auto opt_rigid_body = rigid_body_view->link_rigid_body(rigid_body_handle,
            std::make_unique<BulletRigidBody>(this->resource_pool,
                PhysicsBoneViews{
                    .bone_view = bone_view,
                    .updater = updater,
                    .physics_bone_view = physics_bone_view,
                },
                rigid_body_handle,
                active_motion_state_handle,
                kinematic_motion_state_handle));
        if (opt_rigid_body.is_none()) {
            return foundation::Error(platform::PhysicsError::MakeError);
        }
        auto& rigid_body = opt_rigid_body.unwrap_mut();

        const auto handle = this->handle_mapper->make(types::PhysicsHandleType::RigidBody,
            types::ResourceHandles{
                .resource = rigid_body_handle,
            });

        // Bulletの世界に追加
        this->world->addRigidBody(native_rigid_body.get(), group, mask);

        // オブジェクトとリンク
        this->object_maanger->link_handle(object_handle, handle);

        return handle;
    }

    foundation::Result<types::PhysicsHandle, platform::PhysicsError> PhysicsWorld::add_joint(
        const types::PhysicsHandle& object_handle, types::PhysicsJoint&& joint) noexcept {
        const auto opt_rigid_body_handles =
            this->object_maanger->get_handles(object_handle, types::PhysicsHandleType::RigidBody);
        if (opt_rigid_body_handles.is_none()) {
            return foundation::Error(platform::PhysicsError::MakeError);
        }
        const auto rigid_body_handles = opt_rigid_body_handles.unwrap();
        const size_t size = rigid_body_handles.size();
        const size_t index_rigid_body_a = joint.rigid_body_a;
        const size_t index_rigid_body_b = joint.rigid_body_b;
        // 0 index
        if (index_rigid_body_a == index_rigid_body_b) {
            return foundation::Error(platform::PhysicsError::MakeError);
        }
        if (size < (index_rigid_body_a + 1) || size < (index_rigid_body_b + 1)) {
            return foundation::Error(platform::PhysicsError::MakeError);
        }

        // Jointのインデックスに対応した剛体の取得
        const auto opt_mapped_rigid_body_a =
            this->handle_mapper->get(rigid_body_handles[index_rigid_body_a]);
        const auto opt_mapped_rigid_body_b =
            this->handle_mapper->get(rigid_body_handles[index_rigid_body_b]);
        if (opt_mapped_rigid_body_a.is_none() || opt_mapped_rigid_body_b.is_none()) {
            return foundation::Error(platform::PhysicsError::MakeError);
        }
        const auto& mapped_rigid_body_a = opt_mapped_rigid_body_a.unwrap();
        const auto& mapped_rigid_body_b = opt_mapped_rigid_body_b.unwrap();

        const auto accessor = this->resource_pool->get_native_rigid_body_accessor();
        const auto opt_rigid_body_a = accessor->get_native_rigid_body(mapped_rigid_body_a.resource);
        const auto opt_rigid_body_b = accessor->get_native_rigid_body(mapped_rigid_body_b.resource);
        const auto& rigid_body_a = opt_rigid_body_a.unwrap();
        const auto& rigid_body_b = opt_rigid_body_b.unwrap();

        auto result_joint =
            PhysicsNativeResourceMaker::make_joint(joint, rigid_body_a.get(), rigid_body_b.get());
        if (result_joint.is_err()) {
            return foundation::Error(platform::PhysicsError::MakeError);
        }
        auto& native_joint = result_joint.unwrap_mut();

        // Bulletの世界に追加
        this->world->addConstraint(native_joint.get());

        const auto [resource_handle, _] =
            this->resource_pool->get_native_joint_accessor()->emplace_native_joint(
                std::move(native_joint));

        const auto handle = this->handle_mapper->make(types::PhysicsHandleType::Joint,
            types::ResourceHandles{
                .resource = resource_handle,
            });

        // オブジェクトとリンク
        this->object_maanger->link_handle(object_handle, handle);

        return handle;
    }

    void PhysicsWorld::reset_physics(platform::IBoneUpdater* const updater) {
        const auto rigid_bodies =
            this->resource_pool->get_native_rigid_body_accessor()->get_rigid_bodies();

        for (auto& rb : rigid_bodies) {
            rb->set_active(false);
            rb->reset_transform();
        }
        for (auto& rb : rigid_bodies) {
            rb->apply_global_transform();
        }
        for (auto& rb : rigid_bodies) {
            rb->apply_local_transform();
        }
        /*
        for (auto& node : this->root_nodes) {
            node->update_global();
        }
        */
        for (auto& rb : rigid_bodies) {
            const auto cache = world->getPairCache();
            if (cache != nullptr) {
                /*
                cache->cleanProxyFromPairs(
                    rigid_body->getBroadphaseHandle(), this->world->getDispatcher());
                */
            }
            rb->reset();
        }
    }

    void PhysicsWorld::apply_physics(platform::IBoneUpdater* const updater) {
        const auto rigid_bodies =
            this->resource_pool->get_native_rigid_body_accessor()->get_rigid_bodies();
        for (auto& rb : rigid_bodies) {
            rb->apply_global_transform();
        }
        for (auto& rb : rigid_bodies) {
            rb->apply_local_transform();
        }
        /*
        for (auto& node : this->root_nodes) {
            node->update_global();
        }
        */
    }

    platform::IPhysicsWorldConfigWriter* enishi::physics::bullet3::PhysicsWorld::get_config_writer(
        void) noexcept {
        return this->config.get();
    }

    const platform::IPhysicsWorldConfigReader*
    enishi::physics::bullet3::PhysicsWorld::get_config_reader(void) const noexcept {
        return this->config.get();
    }
} // namespace enishi::physics::bullet3