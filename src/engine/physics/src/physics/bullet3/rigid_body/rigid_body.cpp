#include "rigid_body.h"
#include <glm/gtc/matrix_transform.hpp>
#include <physics/bullet3/motion_state/mmd_dynamic_and_bone_merge_motion_state.h>
#include <physics/bullet3/motion_state/mmd_dynamic_motion_state.h>
#include <physics/bullet3/motion_state/mmd_kinematic_motion_state.h>

namespace enishi::physics::bullet3 {
    BulletRigidBody::BulletRigidBody(void)
        : active_motion_state()
        , kinematic_motion_state()
        , rigid_body() {
    }

    foundation::Result<std::unique_ptr<BulletRigidBody>, PhysicsError> BulletRigidBody::make(
        types::PhysicsRigidBody&& rb,
        std::shared_ptr<platform::IBoneAccessor> bone_node,
        const platform::IBoneListAccessor* bone_list) {
        const bool is_kinematic = rb.kind == types::RigidBodyKind::Kinematic;
        const btScalar mass = is_kinematic ? 0.0f : rb.mass;
        btVector3 local_inertia(0, 0, 0);

        auto&& shape = BulletRigidBody::make_shape(rb);
        auto&& rigid_body = std::make_unique<BulletRigidBody>();
        if (mass != 0.0f) {
            shape->calculateLocalInertia(mass, local_inertia);
        }

        auto [active_motion_state, kinematic_motion_state] =
            BulletRigidBody::make_motion_state(rb, bone_node.get(), bone_list);
        if (!bool(active_motion_state) && !bool(kinematic_motion_state)) {
            return foundation::Error(PhysicsError::MotionStateError);
        }

        btMotionState* const motion_state =
            is_kinematic ? kinematic_motion_state.get() : active_motion_state.get();
        btRigidBody::btRigidBodyConstructionInfo construct_info(
            mass, motion_state, shape.get(), local_inertia);
        construct_info.m_linearDamping = rb.linear_damping;
        construct_info.m_angularDamping = rb.angular_damping;
        construct_info.m_restitution = rb.restitution;
        construct_info.m_friction = rb.friction;
        construct_info.m_additionalDamping = true;

        auto&& bullet_rigid_body = std::make_unique<btRigidBody>(construct_info);
        bullet_rigid_body->setSleepingThresholds(0.01f, glm::radians(0.1f));
        bullet_rigid_body->setUserPointer(rigid_body.get());
        bullet_rigid_body->setActivationState(DISABLE_DEACTIVATION);

        if (is_kinematic) {
            bullet_rigid_body->setCollisionFlags(
                bullet_rigid_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        }

        rigid_body->shape = std::move(shape);
        rigid_body->active_motion_state = std::move(active_motion_state);
        rigid_body->kinematic_motion_state = std::move(kinematic_motion_state);

        return std::move(rigid_body);
    }

    std::unique_ptr<btCollisionShape> BulletRigidBody::make_shape(
        const types::PhysicsRigidBody& rb) {
        if (auto shape = std::get_if<types::RBShapeBox>(&rb.shape)) {
            return std::make_unique<btCollisionShape>(shape->width, shape->height, shape->depth);
        }
        if (auto shape = std::get_if<types::RBShapeCapsule>(&rb.shape)) {
            return std::make_unique<btCollisionShape>(shape->height, shape->raius);
        }
        if (auto shape = std::get_if<types::RBShapeSphere>(&rb.shape)) {
            return std::make_unique<btCollisionShape>(shape->radius);
        }

        return std::unique_ptr<btCollisionShape>();
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

    void BulletRigidBody::reset(btDiscreteDynamicsWorld* const world) {
        const auto cache = world->getPairCache();
        if (cache != nullptr) {
            cache->cleanProxyFromPairs(
                this->rigid_body->getBroadphaseHandle(), world->getDispatcher());
        }
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
            const auto parent_node = opt_parent.unwrap();
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

    // PMDはボーンとの相対座標なので剛体中心とのオフセットは以下で求める(列優先の場合)
    // Offset = T * R
    // PMXの場合はモデル座標での数値なので以下で求める(列優先の場合)
    // Offset = Inverse(global) * T * R
    glm::mat4 BulletRigidBody::make_offset(
        const types::PhysicsRigidBody& rigid_body, const platform::IBoneAccessor* node) {
        const glm::mat4 rx =
            glm::rotate(glm::mat4{1.0f}, rigid_body.rotation.x, glm::vec3{1.0f, 0.0f, 0.0f});
        const glm::mat4 ry =
            glm::rotate(glm::mat4{1.0f}, rigid_body.rotation.y, glm::vec3{0.0f, 1.0f, 0.0f});
        const glm::mat4 rz =
            glm::rotate(glm::mat4{1.0f}, rigid_body.rotation.z, glm::vec3{0.0f, 0.0f, 1.0f});
        const glm::mat4 rotate_matrix = ry * rx * rz;
        const glm::mat4 translate_matrix = glm::translate(glm::mat4{1.0f}, rigid_body.position);
        glm::mat4&& offset = translate_matrix * rotate_matrix;

        return offset;
    }

    std::tuple<BulletRigidBody::MotionState, BulletRigidBody::MotionState>
    BulletRigidBody::make_motion_state(const types::PhysicsRigidBody& rigid_body,
        const platform::IBoneAccessor* node,
        const platform::IBoneListAccessor* bone_list) {
        const bool has_node = bool(node);
        const auto opt_root_node = bone_list->get_bone_accessor(0);
        if (opt_root_node.is_none()) {
            return {};
        }
        const auto root_node = opt_root_node.unwrap();
        const auto& kinematic_node = has_node ? node : root_node;

        const auto offset = BulletRigidBody::make_offset(rigid_body, kinematic_node);
        const bool override_with_physics = has_node;

        MotionState active_motion_state;
        MotionState kinematic_motion_state;
        switch (rigid_body.kind) {
            case types::RigidBodyKind::Kinematic:
                kinematic_motion_state =
                    std::make_unique<MMDKinematicMotionState>(kinematic_node, offset);
                break;
            case types::RigidBodyKind::Dynamic:
                active_motion_state = std::make_unique<MMDDynamicMotionState>(
                    kinematic_node, offset, override_with_physics);
                kinematic_motion_state =
                    std::make_unique<MMDKinematicMotionState>(kinematic_node, offset);
                break;
            case types::RigidBodyKind::DynamicAdjustBone:
                active_motion_state = std::make_unique<MMDDynamicAndBoneMergeMotionState>(
                    kinematic_node, offset, override_with_physics);
                kinematic_motion_state =
                    std::make_unique<MMDKinematicMotionState>(kinematic_node, offset);
                break;
            default:
                return {};
        }

        return {
            std::move(active_motion_state),
            std::move(kinematic_motion_state),
        };
    }
} // namespace enishi::physics::bullet3