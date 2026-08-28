#include "mmd_kinematic_motion_state.h"
#include <physics/bullet3/converter/bullet_converter.h>
#include <physics/helper/helper.h>

namespace enishi::physics::bullet3 {
    MMDKinematicMotionState::MMDKinematicMotionState(
        std::shared_ptr<platform::IBoneUpdater> bone_node, const glm::mat4& offset)
        : bone_node(bone_node)
        , offset(offset) {
    }
    void enishi::physics::bullet3::MMDKinematicMotionState::getWorldTransform(
        btTransform& worldTrans) const {
        const auto& global = this->bone_node->get_bone_global();
        const auto& offset_matrix = global * this->offset;
        worldTrans = BulletConverter::matrix_to_transform(inverse_z(offset_matrix));
    }

    void MMDKinematicMotionState::setWorldTransform(const btTransform& worldTrans) {
    }

    void MMDKinematicMotionState::reset(void) {
    }

    void MMDKinematicMotionState::reflect_global_transform(void) {
    }
} // namespace enishi::physics::bullet3