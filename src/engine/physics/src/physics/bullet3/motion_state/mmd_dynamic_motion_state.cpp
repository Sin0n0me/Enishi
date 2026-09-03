#include "mmd_dynamic_motion_state.h"
#include <physics/bullet3/converter/bullet_converter.h>
#include <physics/helper/helper.h>

namespace enishi::physics::bullet3 {
    MMDDynamicMotionState::MMDDynamicMotionState(
        const glm::mat4& offset, const bool override_with_physics)
        : global(glm::mat4{1.0f})
        , offset(offset)
        , inverse_offset(glm::inverse(offset))
        , override_with_physics(override_with_physics)
        , transform(btTransform::getIdentity()) {
    }

    void enishi::physics::bullet3::MMDDynamicMotionState::getWorldTransform(
        btTransform& worldTrans) const {
        worldTrans = this->transform;
    }

    void MMDDynamicMotionState::setWorldTransform(const btTransform& worldTrans) {
        this->transform = worldTrans;
    }

    void MMDDynamicMotionState::set_offset(const glm::mat4& offset) {
        this->offset = offset;
        this->inverse_offset = glm::inverse(offset);
    }

    void MMDDynamicMotionState::reset(platform::IBoneUpdater* const bone_updater) {
        // mmdの世界からbulletの世界に変換しオフセット適用
        this->global = bone_updater->get_bone_global();
        const auto offset_matrix = this->global * this->offset;
        this->transform = BulletConverter::matrix_to_transform(inverse_z(offset_matrix));
    }

    void MMDDynamicMotionState::update_global_transform(
        platform::IBoneUpdater* const bone_updater) {
        this->global = bone_updater->get_bone_global();
    }

    void MMDDynamicMotionState::reflect_global_transform(
        platform::IBoneUpdater* const bone_updater) {
        if (!this->override_with_physics) {
            return;
        }

        // 行優先計算
        // 中心とのoffsetが掛かっているので
        // offsetの逆行列を掛けることでボーン空間に戻す
        auto&& global =
            inverse_z(BulletConverter::transform_to_matrix(this->transform)) * this->inverse_offset;

        // MMDの世界に変換
        bone_updater->set_bone_global(std::move(global));
        bone_updater->update_children_global();
    }
} // namespace enishi::physics::bullet3