#include "rigid_body.h"
#include <utility>

namespace enishi::physics::bullet3 {
    BulletRigidBody::BulletRigidBody(btRigidBody& rigid_body,
        IMMDMotionState& active_motion_state,
        IMMDMotionState& kinematic_motion_state,
        PhysicsBoneViews&& views,
        const types::RigidBodyKind kind,
        const types::BoneIndex bone_index)
        : rigid_body(rigid_body)
        , active_motion_state(active_motion_state)
        , kinematic_motion_state(kinematic_motion_state)
        , views(std::move(views))
        , kind(kind)
        , bone_index(bone_index) {
    }

    void BulletRigidBody::set_active(const bool active_flag) {
        const auto current_flag = this->rigid_body.getCollisionFlags();
        const bool dynamic = active_flag && this->kind != types::RigidBodyKind::Kinematic;
        const auto flag = dynamic ? current_flag & ~btCollisionObject::CF_KINEMATIC_OBJECT
                                  : current_flag | btCollisionObject::CF_KINEMATIC_OBJECT;
        this->rigid_body.setCollisionFlags(flag);
        this->rigid_body.setMotionState(
            dynamic ? &this->active_motion_state : &this->kinematic_motion_state);
        this->rigid_body.activate(true);
    }

    void BulletRigidBody::reset(void) {
        this->rigid_body.setAngularVelocity(btVector3(0, 0, 0));
        this->rigid_body.setLinearVelocity(btVector3(0, 0, 0));
        this->rigid_body.clearForces();
    }

    void BulletRigidBody::reset_transform(void) {
        auto& state = this->kind == types::RigidBodyKind::Kinematic ? this->kinematic_motion_state
                                                                    : this->active_motion_state;
        state.reset(this->views.physics_bone_view.get());
        btTransform transform;
        this->rigid_body.getMotionState()->getWorldTransform(transform);
        this->rigid_body.setWorldTransform(transform);
        this->rigid_body.setInterpolationWorldTransform(transform);
        this->rigid_body.activate(true);
        this->has_animation_transform = false;
    }

    void BulletRigidBody::sync_animation(void) {
        if (!this->is_in_world()) {
            return;
        }
        const auto& animated = this->views.physics_bone_view->get_physics_global();
        // Compare authored input, not the simulated transform: settling must not wake itself.
        if (!this->has_animation_transform || animated != this->animation_transform) {
            this->rigid_body.activate(true);
            this->animation_transform = animated;
            this->has_animation_transform = true;
        }
        this->kinematic_motion_state.update_global_transform(this->views.physics_bone_view.get());
        this->active_motion_state.update_global_transform(this->views.physics_bone_view.get());
    }

    void BulletRigidBody::apply_local_transform(void) {
        if (!this->is_in_world()) {
            return;
        }
        this->views.updater->update_local(this->bone_index);
    }

    void BulletRigidBody::apply_global_transform(void) {
        if (!this->is_in_world()) {
            return;
        }
        this->active_motion_state.reflect_global_transform(
            this->views.physics_bone_view.get(), this->views.updater.get());
        this->kinematic_motion_state.reflect_global_transform(
            this->views.physics_bone_view.get(), this->views.updater.get());
    }

    bool BulletRigidBody::is_in_world(void) const {
        return this->rigid_body.isInWorld();
    }
} // namespace enishi::physics::bullet3
