#include "mmd_kinematic_motion_state.h"
#include <physics/bullet3/converter/bullet_converter.h>
#include <physics/helper/helper.h>

namespace enishi::physics::bullet3 {
    MMDKinematicMotionState::MMDKinematicMotionState(const glm::mat4& offset) noexcept
        : global(glm::mat4{1.0f})
        , offset(offset)
        , transform(btTransform::getIdentity()) {
    }

    void enishi::physics::bullet3::MMDKinematicMotionState::getWorldTransform(
        btTransform& worldTrans) const {
        const auto& offset_matrix = this->global * this->offset;
        worldTrans = BulletConverter::matrix_to_transform(inverse_z(offset_matrix));
    }

    void MMDKinematicMotionState::setWorldTransform(const btTransform& worldTrans) {
    }

    void MMDKinematicMotionState::reset(platform::IPhysicsBoneView* const physics_bone) {
        this->update_global_transform(physics_bone);
    }

    void MMDKinematicMotionState::set_offset(const glm::mat4& offset) {
        this->offset = offset;
    }

    void MMDKinematicMotionState::update_global_transform(
        platform::IPhysicsBoneView* const physics_bone) {
        this->global = physics_bone->get_physics_global();
    }

    void MMDKinematicMotionState::reflect_global_transform(
        platform::IPhysicsBoneView* const physics_bone,
        platform::IBoneUpdater* const bone_updater) {
        // 物理エンジンからの影響を受けない
    }
} // namespace enishi::physics::bullet3