#include "rigid_body.h"

namespace enishi::physics::bullet3 {
    BulletRigidBody::BulletRigidBody(void)
        : active_motion_state()
        , kinematic_motion_state()
        , rigid_body() {
    }

    void BulletRigidBody::set_active(const bool active_flag) {
        const auto current_flag = this->rigid_body->getCollisionFlags();
        if (this->rigid_body_description.kind != types::RigidBodyKind::Kinematic) {
            const auto motion_state =
                active_flag ? this->active_motion_state.get() : this->kinematic_motion_state.get();
            const auto flag = active_flag ? current_flag & ~btCollisionObject::CF_KINEMATIC_OBJECT
                                          : current_flag | btCollisionObject::CF_KINEMATIC_OBJECT;

            this->rigid_body->setCollisionFlags(flag);
            this->rigid_body->setMotionState(motion_state);
        } else {
            const auto flag = current_flag | btCollisionObject::CF_KINEMATIC_OBJECT;
            this->rigid_body->setCollisionFlags(flag);
            this->rigid_body->setMotionState(this->kinematic_motion_state.get());
        }
    }

    void BulletRigidBody::reset(void) {
        /*
        const auto cache = world->getPairCache();
        if (cache != nullptr) {
            cache->cleanProxyFromPairs(
                this->rigid_body->getBroadphaseHandle(), world->getDispatcher());
        }
        btDiscreteDynamicsWorld* const world
        */
        this->rigid_body->setAngularVelocity(btVector3(0, 0, 0));
        this->rigid_body->setLinearVelocity(btVector3(0, 0, 0));
        this->rigid_body->clearForces();
    }

    void BulletRigidBody::reset_transform(void) {
        if (bool(this->active_motion_state)) {
            this->active_motion_state->reset();
        }
    }

    void BulletRigidBody::apply_local_transform(void) {
        if (!bool(this->bone_node)) {
            return;
        }

        // 物理状態の反映(ローカル空間)
        const auto& global = this->bone_node->get_bone_global();
        const auto opt_parent = this->bone_node->get_parent_accessor();
        if (opt_parent.is_some()) {
            const auto& parent_node = opt_parent.unwrap();
            const auto& parent_global = parent_node->get_bone_global();
            const auto& local = glm::inverse(parent_global) * global;

            this->bone_node->get_updater()->set_bone_local(local);
        } else {
            this->bone_node->get_updater()->set_bone_local(global);
        }
    }

    void BulletRigidBody::apply_global_transform(void) {
        if (bool(this->active_motion_state)) {
            this->active_motion_state->reflect_global_transform();
        }
        if (bool(this->kinematic_motion_state)) {
            this->kinematic_motion_state->reflect_global_transform();
        }
    }

    const types::PhysicsRigidBody& BulletRigidBody::get_rigid_body(void) const noexcept {
        return this->rigid_body_description;
    }
} // namespace enishi::physics::bullet3