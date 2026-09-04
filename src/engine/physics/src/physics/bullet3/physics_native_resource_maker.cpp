#include "physics_native_resource_maker.h"
#include <glm/gtc/matrix_transform.hpp>
#include <physics/bullet3/motion_state/mmd_dynamic_and_bone_merge_motion_state.h>
#include <physics/bullet3/motion_state/mmd_dynamic_motion_state.h>
#include <physics/bullet3/motion_state/mmd_kinematic_motion_state.h>

namespace enishi::physics::bullet3 {
    foundation::Result<PhysicsNativeResourceMaker::Shape, PhysicsError>
    PhysicsNativeResourceMaker::make_shape(const types::PhysicsRigidBody& rb) {
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

    std::tuple<PhysicsNativeResourceMaker::MotionState, PhysicsNativeResourceMaker::MotionState>
    PhysicsNativeResourceMaker::make_motion_state(
        const types::PhysicsRigidBody& rigid_body, const bool has_bone) {
        const auto offset = PhysicsNativeResourceMaker::make_offset(rigid_body);
        MotionState active_motion_state;
        MotionState kinematic_motion_state;
        switch (rigid_body.kind) {
            case types::RigidBodyKind::Kinematic: {
                kinematic_motion_state = std::make_unique<MMDKinematicMotionState>(offset);
            } break;
            case types::RigidBodyKind::Dynamic: {
                active_motion_state = std::make_unique<MMDDynamicMotionState>(offset, has_bone);
                kinematic_motion_state = std::make_unique<MMDKinematicMotionState>(offset);
            } break;
            case types::RigidBodyKind::DynamicAdjustBone: {
                active_motion_state =
                    std::make_unique<MMDDynamicAndBoneMergeMotionState>(offset, has_bone);
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

    foundation::Result<PhysicsNativeResourceMaker::RigidBody, PhysicsError>
    PhysicsNativeResourceMaker::make_rigid_body(types::PhysicsRigidBody&& rigid_body,
        btCollisionShape* const shape,
        IMMDMotionState* const active_motion_state,
        IMMDMotionState* const kinematic_motion_state) {
        auto&& native_rigid_body = std::make_unique<btRigidBody>();

        const bool is_kinematic = rigid_body.kind == types::RigidBodyKind::Kinematic;
        const btScalar mass = is_kinematic ? 0.0f : rigid_body.mass;
        btVector3 local_inertia(0, 0, 0);
        if (mass != 0.0f) {
            shape->calculateLocalInertia(mass, local_inertia);
        }

        btMotionState* const motion_state =
            is_kinematic ? kinematic_motion_state : active_motion_state;
        btRigidBody::btRigidBodyConstructionInfo construct_info(
            mass, motion_state, shape, local_inertia);
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

        return native_rigid_body;
    }

    foundation::Result<std::unique_ptr<btGeneric6DofSpringConstraint>, PhysicsError>
    PhysicsNativeResourceMaker::make_joint(const types::PhysicsJoint& joint,
        btRigidBody* const rigid_body_a,
        btRigidBody* const rigid_body_b) {
        btMatrix3x3 rotate_matrix;
        rotate_matrix.setEulerZYX(joint.rotation.x, joint.rotation.y, joint.rotation.z);

        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(joint.position.x,
            joint.position.y,
            -joint.position.z // Bulletに合わせる
            ));
        transform.setBasis(rotate_matrix);

        const btTransform inverse_a = rigid_body_a->getWorldTransform().inverse() * transform;
        const btTransform inverse_b = rigid_body_b->getWorldTransform().inverse() * transform;

        auto&& native = std::make_unique<btGeneric6DofSpringConstraint>(
            *rigid_body_a, *rigid_body_b, inverse_a, inverse_b, true);

        auto&& result = PhysicsNativeResourceMaker::set_joint(native.get(), joint);
        if (result.is_err()) {
            return std::move(result).unwrap_err();
        }

        return native;
    }

    foundation::VoidResult<PhysicsError> PhysicsNativeResourceMaker::set_joint(
        btGeneric6DofSpringConstraint* const constraint, const types::PhysicsJoint& joint) {
        constraint->setLinearLowerLimit(btVector3(joint.constrain_position_min[0],
            joint.constrain_position_min[1],
            joint.constrain_position_min[2]));
        constraint->setLinearUpperLimit(btVector3(joint.constrain_position_max[0],
            joint.constrain_position_max[1],
            joint.constrain_position_max[2]));

        constraint->setAngularLowerLimit(btVector3(joint.constrain_rotation_min[0],
            joint.constrain_rotation_min[1],
            joint.constrain_rotation_min[2]));
        constraint->setAngularUpperLimit(btVector3(joint.constrain_rotation_max[0],
            joint.constrain_rotation_max[1],
            joint.constrain_rotation_max[2]));

        if (joint.spring_position[0] != 0) {
            constraint->enableSpring(0, true);
            constraint->setStiffness(0, joint.spring_position[0]);
        }
        if (joint.spring_position[1] != 0) {
            constraint->enableSpring(1, true);
            constraint->setStiffness(1, joint.spring_position[1]);
        }
        if (joint.spring_position[2] != 0) {
            constraint->enableSpring(2, true);
            constraint->setStiffness(2, -joint.spring_position[2]); // Bulletに合わせる
        }
        if (joint.spring_rotation[0] != 0) {
            constraint->enableSpring(3, true);
            constraint->setStiffness(3, joint.spring_rotation[0]);
        }
        if (joint.spring_rotation[1] != 0) {
            constraint->enableSpring(4, true);
            constraint->setStiffness(4, joint.spring_rotation[1]);
        }
        if (joint.spring_rotation[2] != 0) {
            constraint->enableSpring(5, true);
            constraint->setStiffness(5, joint.spring_rotation[2]);
        }

        return {};
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
} // namespace enishi::physics::bullet3