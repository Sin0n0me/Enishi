#include "physics_native_resource_maker.h"
#include <glm/gtc/matrix_transform.hpp>
#include <physics/bullet3/motion_state/mmd_dynamic_and_bone_merge_motion_state.h>
#include <physics/bullet3/motion_state/mmd_dynamic_motion_state.h>
#include <physics/bullet3/motion_state/mmd_kinematic_motion_state.h>

namespace enishi::physics::bullet3 {
    foundation::Result<std::unique_ptr<btRigidBody>, PhysicsError>
    PhysicsNativeResourceMaker::set_rigid_body(
        btRigidBody* const native_rigid_body, types::PhysicsRigidBody&& rigid_body) {
        const bool is_kinematic = rigid_body.kind == types::RigidBodyKind::Kinematic;
        const btScalar mass = is_kinematic ? 0.0f : rigid_body.mass;
        btVector3 local_inertia(0, 0, 0);

        auto&& shape = PhysicsNativeResourceMaker::make_shape(rigid_body);
        if (mass != 0.0f) {
            shape->calculateLocalInertia(mass, local_inertia);
        }

        auto [active_motion_state, kinematic_motion_state] =
            PhysicsNativeResourceMaker::make_motion_state(rigid_body);
        if (!bool(active_motion_state) && !bool(kinematic_motion_state)) {
            return foundation::Error(PhysicsError::MotionStateError);
        }

        btMotionState* const motion_state =
            is_kinematic ? kinematic_motion_state.get() : active_motion_state.get();
        btRigidBody::btRigidBodyConstructionInfo construct_info(
            mass, motion_state, shape.get(), local_inertia);
        construct_info.m_linearDamping = rigid_body.linear_damping;
        construct_info.m_angularDamping = rigid_body.angular_damping;
        construct_info.m_restitution = rigid_body.restitution;
        construct_info.m_friction = rigid_body.friction;
        construct_info.m_additionalDamping = true;

        auto&& bullet_rigid_body = std::make_unique<btRigidBody>(construct_info);
        bullet_rigid_body->setSleepingThresholds(0.01f, glm::radians(0.1f));
        // bullet_rigid_body->setUserPointer();
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

    std::unique_ptr<btCollisionShape> PhysicsNativeResourceMaker::make_shape(
        const types::PhysicsRigidBody& rb) {
        if (auto shape = std::get_if<types::RBShapeBox>(&rb.shape)) {
            return std::make_unique<btBoxShape>(btVector3{
                shape->width,
                shape->height,
                shape->depth,
            });
        }
        if (auto shape = std::get_if<types::RBShapeCapsule>(&rb.shape)) {
            return std::make_unique<btCapsuleShape>(shape->height, shape->raius);
        }
        if (auto shape = std::get_if<types::RBShapeSphere>(&rb.shape)) {
            return std::make_unique<btSphereShape>(shape->radius);
        }

        return std::unique_ptr<btCollisionShape>();
    }

    // PMDはボーンとの相対座標なので剛体中心とのオフセットは以下で求める(列優先の場合)
    // Offset = T * R
    // PMXの場合はモデル座標での数値なので以下で求める(列優先の場合)
    // Offset = Inverse(global) * T * R
    glm::mat4 PhysicsNativeResourceMaker::make_offset(const types::PhysicsRigidBody& rigid_body) {
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

    std::tuple<PhysicsNativeResourceMaker::MotionState, PhysicsNativeResourceMaker::MotionState>
    PhysicsNativeResourceMaker::make_motion_state(const types::PhysicsRigidBody& rigid_body) {
        const bool has_node = bool(node);
        const auto opt_root_node = bone_list->get_bone_accessor(0);
        if (opt_root_node.is_none()) {
            return {};
        }
        const auto& kinematic_node = has_node ? node : opt_root_node.unwrap();
        const auto updater = kinematic_node->get_updater();

        const auto offset = PhysicsNativeResourceMaker::make_offset(rigid_body);
        const bool override_with_physics = has_node;

        MotionState active_motion_state;
        MotionState kinematic_motion_state;
        switch (rigid_body.kind) {
            case types::RigidBodyKind::Kinematic: {
                kinematic_motion_state = std::make_unique<MMDKinematicMotionState>(offset);
            } break;
            case types::RigidBodyKind::Dynamic: {
                active_motion_state =
                    std::make_unique<MMDDynamicMotionState>(offset, override_with_physics);
                kinematic_motion_state = std::make_unique<MMDKinematicMotionState>(offset);
            } break;
            case types::RigidBodyKind::DynamicAdjustBone: {
                active_motion_state = std::make_unique<MMDDynamicAndBoneMergeMotionState>(
                    offset, override_with_physics);
                kinematic_motion_state = std::make_unique<MMDKinematicMotionState>(offset);
            } break;
            default:
                return {};
        }

        return {
            std::move(active_motion_state),
            std::move(kinematic_motion_state),
        };
    }
} // namespace enishi::physics::bullet3