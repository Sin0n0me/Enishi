#include "rigid_body.h"
#include <physics/bullet3/physics_native_resource_maker.h>

namespace enishi::physics::bullet3 {
    BulletRigidBody::BulletRigidBody(std::shared_ptr<INativePhysicsAccessor> view,
        std::shared_ptr<platform::IBoneView> bone_view,
        std::shared_ptr<platform::IBoneUpdater> updater,
        std::shared_ptr<platform::IPhysicsBoneView> physics_bone_view)
        : view(view)
        , bone_view(bone_view)
        , updater(updater)
        , physics_bone_view(physics_bone_view) {
    }

    void BulletRigidBody::set_active(const bool active_flag) {
        auto&& opt_native_rigid_body =
            this->view->get_native_rigid_body_accessor()->get_native_rigid_body(this->rigid_body);
        if (opt_native_rigid_body.is_none()) {
            return;
        }

        auto&& opt_active_motion_state =
            this->view->get_native_motion_state_accessor()->get_native_motion_state(
                this->active_motion_state);
        auto&& opt_kinematic_motion_state =
            this->view->get_native_motion_state_accessor()->get_native_motion_state(
                this->kinematic_motion_state);
        if (opt_kinematic_motion_state.is_none() || opt_active_motion_state.is_none()) {
            return;
        }
        auto&& native_rigid_body = opt_native_rigid_body.unwrap();
        auto&& active_motion_state = opt_active_motion_state.unwrap();
        auto&& kinematic_motion_state = opt_kinematic_motion_state.unwrap();

        const auto current_flag = native_rigid_body->getCollisionFlags();
        if (this->kind != types::RigidBodyKind::Kinematic) {
            const auto motion_state =
                active_flag ? active_motion_state.get() : kinematic_motion_state.get();
            const auto flag = active_flag ? current_flag & ~btCollisionObject::CF_KINEMATIC_OBJECT
                                          : current_flag | btCollisionObject::CF_KINEMATIC_OBJECT;

            native_rigid_body->setCollisionFlags(flag);
            native_rigid_body->setMotionState(motion_state);
        } else {
            const auto flag = current_flag | btCollisionObject::CF_KINEMATIC_OBJECT;
            native_rigid_body->setCollisionFlags(flag);
            native_rigid_body->setMotionState(kinematic_motion_state.get());
        }
    }

    void BulletRigidBody::reset(void) {
        auto&& opt_native_rigid_body =
            this->view->get_native_rigid_body_accessor()->get_native_rigid_body(this->rigid_body);
        if (opt_native_rigid_body.is_none()) {
            return;
        }
        auto&& native_rigid_body = opt_native_rigid_body.unwrap_mut();
        /*
        const auto cache = world->getPairCache();
        if (cache != nullptr) {
            cache->cleanProxyFromPairs(
                this->rigid_body->getBroadphaseHandle(), world->getDispatcher());
        }
        btDiscreteDynamicsWorld* const world
        */
        native_rigid_body->setAngularVelocity(btVector3(0, 0, 0));
        native_rigid_body->setLinearVelocity(btVector3(0, 0, 0));
        native_rigid_body->clearForces();
    }

    void BulletRigidBody::reset_transform(void) {
        auto&& opt_active_motion_state =
            this->view->get_native_motion_state_accessor()->get_native_motion_state(
                this->active_motion_state);
        if (opt_active_motion_state.is_some()) {
            auto&& active_motion_state = opt_active_motion_state.unwrap();
            active_motion_state->reset(this->physics_bone_view.get());
        }
    }

    void BulletRigidBody::apply_local_transform(void) {
        // 物理状態の反映(ローカル空間)
        const auto opt_parent = this->bone_view->get_parent_view();
        const auto& global = this->physics_bone_view->get_physics_global();
        if (opt_parent.is_some()) {
            const auto& parent = opt_parent.unwrap();
            const auto& parent_global = parent->get_physics_bone().global;
            const auto& local = glm::inverse(parent_global) * global;

            this->physics_bone_view->set_physics_local(local);
        } else {
            this->physics_bone_view->set_physics_local(global);
        }
    }

    void BulletRigidBody::apply_global_transform(void) {
        auto&& opt_active_motion_state =
            this->view->get_native_motion_state_accessor()->get_native_motion_state(
                this->active_motion_state);
        auto&& opt_kinematic_motion_state =
            this->view->get_native_motion_state_accessor()->get_native_motion_state(
                this->kinematic_motion_state);

        if (opt_active_motion_state.is_some()) {
            auto&& active_motion_state = opt_active_motion_state.unwrap();
            active_motion_state->reflect_global_transform(
                this->physics_bone_view.get(), this->updater.get());
        }
        if (opt_kinematic_motion_state.is_some()) {
            auto&& kinematic_motion_state = opt_kinematic_motion_state.unwrap();
            kinematic_motion_state->reflect_global_transform(
                this->physics_bone_view.get(), this->updater.get());
        }
    }
} // namespace enishi::physics::bullet3