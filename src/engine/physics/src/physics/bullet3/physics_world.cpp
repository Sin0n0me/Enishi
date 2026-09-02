#include "physics_world.h"
#include "joint/joint.h"
#include "rigid_body/rigid_body.h"

namespace enishi::physics::bullet3 {
    constexpr float GRAVITY = 9.80665f;

    foundation::Result<std::unique_ptr<PhysicsWorld>, PhysicsError> PhysicsWorld::make(void) {
        auto physics_world = std::make_unique<PhysicsWorld>();
        auto& world = physics_world->world;

        auto& info = world->getSolverInfo();
        info.m_numIterations = 10;
        info.m_solverMode = btSolverMode::SOLVER_SIMD;

        // 重力設定
        world->setGravity(btVector3(0, -GRAVITY * 2.0f, 0));

        // 地面作成
        physics_world->ground_shape =
            std::make_unique<btStaticPlaneShape>(btVector3(0, 1, 0), 0.0f);
        physics_world->ground_motion_state =
            std::make_unique<btDefaultMotionState>(btTransform::getIdentity());
        physics_world->ground_rigid_body =
            std::make_unique<btRigidBody>(btRigidBody::btRigidBodyConstructionInfo(
                0, this->ground_motion_state.get(), this->ground_shape.get(), btVector3(0, 0, 0)));

        world->addRigidBody(this->ground_rigid_body.get());

        {
            auto filterCB = std::make_unique<MMDFilterCallback>();
            filterCB->add_proxy(this->ground_rigid_body->getBroadphaseProxy());
            this->filter_callback = std::move(filterCB);
        }
        world->getPairCache()->setOverlapFilterCallback(this->filter_callback.get());

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
        auto result_rigid_body = BulletRigidBody::make(rigid_body, bone_accessor, node);
        if (result_rigid_body.is_err()) {
            return;
        }
        auto& model_rigid_body = result_rigid_body.unwrap();

        // Bulletの世界に追加
        const auto& description = model_rigid_body->get_rigid_body();
        this->world->addRigidBody(model_rigid_body->get_native_rigid_body(),
            1 << description.group_index,
            description.group_target);

        this->rigid_bodies.emplace_back(std::move(mmd_rigid_body));

        return types::PhysicsHandle();
    }

    types::PhysicsHandle PhysicsWorld::add_joint(types::PhysicsJoint&& joint) {
        /*
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

        // Bulletの世界に追加
        // ポインタのコピー(参照不可)
        const auto bt_joint = mmd_joint.get();
        this->world->addConstraint(bt_joint->get_constraint());

        this->joints.emplace_back(std::move(mmd_joint));
        */

        return types::PhysicsHandle();
    }
} // namespace enishi::physics::bullet3