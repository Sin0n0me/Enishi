#include "rigid_body.h"
#include <physics/bullet3/physics_native_resource_maker.h>

namespace enishi::physics::bullet3 {
    BulletRigidBody::BulletRigidBody(std::shared_ptr<INativePhysicsAccessor> native_view,
        PhysicsBoneViews&& views,
        types::HandleId rigid_body,
        types::HandleId active_motion_state,
        types::HandleId kinematic_motion_state)
        : native_view(native_view)
        , rigid_body(rigid_body)
        , active_motion_state(active_motion_state)
        , kinematic_motion_state(kinematic_motion_state) {
    }

    void BulletRigidBody::set_active(const bool active_flag) {
        auto&& opt_native_rigid_body =
            this->native_view->get_native_rigid_body_accessor()->get_native_rigid_body(
                this->rigid_body);
        if (opt_native_rigid_body.is_none()) {
            return;
        }

        auto&& opt_active_motion_state =
            this->native_view->get_native_motion_state_accessor()->get_native_motion_state(
                this->active_motion_state);
        auto&& opt_kinematic_motion_state =
            this->native_view->get_native_motion_state_accessor()->get_native_motion_state(
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
            this->native_view->get_native_rigid_body_accessor()->get_native_rigid_body(
                this->rigid_body);
        if (opt_native_rigid_body.is_none()) {
            return;
        }
        auto&& native_rigid_body = opt_native_rigid_body.unwrap_mut();
        native_rigid_body->setAngularVelocity(btVector3(0, 0, 0));
        native_rigid_body->setLinearVelocity(btVector3(0, 0, 0));
        native_rigid_body->clearForces();
    }

    void BulletRigidBody::reset_transform(void) {
        auto&& opt_active_motion_state =
            this->native_view->get_native_motion_state_accessor()->get_motion_state(
                this->active_motion_state);
        if (opt_active_motion_state.is_some()) {
            auto&& active_motion_state = opt_active_motion_state.unwrap();
            active_motion_state->reset(this->views.physics_bone_view.get());
        }
    }

    void BulletRigidBody::apply_local_transform(void) {
        // 物理状態の反映(ローカル空間)
        const auto opt_parent = this->views.bone_view->get_parent_view();
        const auto& global = this->views.physics_bone_view->get_physics_global();
        if (opt_parent.is_some()) {
            const auto& parent = opt_parent.unwrap();
            const auto& parent_global = parent->get_physics_bone().global;
            const auto& local = glm::inverse(parent_global) * global;

            this->views.physics_bone_view->set_physics_local(local);
        } else {
            this->views.physics_bone_view->set_physics_local(global);
        }
    }

    void BulletRigidBody::apply_global_transform(void) {
        auto&& opt_active_motion_state =
            this->native_view->get_native_motion_state_accessor()->get_motion_state(
                this->active_motion_state);
        auto&& opt_kinematic_motion_state =
            this->native_view->get_native_motion_state_accessor()->get_motion_state(
                this->kinematic_motion_state);

        if (opt_active_motion_state.is_some()) {
            auto&& active_motion_state = opt_active_motion_state.unwrap();
            active_motion_state->reflect_global_transform(
                this->views.physics_bone_view.get(), this->views.updater.get());
        }
        if (opt_kinematic_motion_state.is_some()) {
            auto&& kinematic_motion_state = opt_kinematic_motion_state.unwrap();
            kinematic_motion_state->reflect_global_transform(
                this->views.physics_bone_view.get(), this->views.updater.get());
        }
    }
} // namespace enishi::physics::bullet3