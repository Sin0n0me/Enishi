#include "mmd_dynamic_and_bone_merge_motion_state.h"
#include <physics/bullet3/converter/bullet_converter.h>
#include <physics/helper/helper.h>

namespace enishi::physics::bullet3 {
    MMDDynamicAndBoneMergeMotionState::MMDDynamicAndBoneMergeMotionState(
        std::shared_ptr<platform::IBoneUpdater> bone_node,
        const glm::mat4& offset,
        const bool override_with_physics)
        : bone_node(bone_node)
        , offset(offset)
        , override_with_physics(override_with_physics) {
    }

    void MMDDynamicAndBoneMergeMotionState::getWorldTransform(btTransform& worldTrans) const {
        worldTrans = this->transform;
    }

    void MMDDynamicAndBoneMergeMotionState::setWorldTransform(const btTransform& worldTrans) {
        this->transform = worldTrans;
    }

    void enishi::physics::bullet3::MMDDynamicAndBoneMergeMotionState::reset(void) {
        // mmdの世界からbulletの世界に変換しオフセット適用
        const auto global = this->bone_node->get_bind_bone().global;
        const auto offset_matrix = global * this->offset;
        this->transform = BulletConverter::matrix_to_transform(inverse_z(offset_matrix));
    }

    void MMDDynamicAndBoneMergeMotionState::reflect_global_transform(void) {
        if (!this->override_with_physics) {
            return;
        }

        // 行優先計算
        // 中心とのoffsetが掛かっているので
        // offsetの逆行列を掛けることでボーン空間に戻す
        auto&& global =
            inverse_z(BulletConverter::transform_to_matrix(this->transform)) * this->inverse_offset;

        // Position
        const auto position_index = 0;
        global[position_index] = this->bone_node->get_bone_global()[position_index];

        this->bone_node->set_bone_global(std::move(global));
        this->bone_node->update_children_global();
    }
} // namespace enishi::physics::bullet3