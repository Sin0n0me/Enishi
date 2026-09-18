#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <engine_types/assets/model/addons/ik.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <sub_system/ik/interface_ik_bone_view_list.h>
#include <sub_system/skinning_system/interface_bone_updater.h>

namespace enishi::ik {
    // 純粋なロジックのみ
    // 状態などを持たない
    class IKSolver {
      public:
        static void apply_ik(const types::IK& ik,
            sub_system::IIKBoneViewList* const ik_view_list,
            sub_system::IBoneUpdater* const updater,
            const types::BoneIndex index);

      private:
        static void ccd_ik(sub_system::IIKBoneViewList* const ik_view_list,
            sub_system::IBoneUpdater* const updater,
            const types::CCDIK& ik,
            const types::BoneIndex index);

        // 制限なしCCD IK
        static glm::quat solve_ik(const glm::mat4& bone_global,
            const glm::mat4& ik_bone_global,
            const glm::mat4& target_bone_global,
            const float ik_limit);

        // 制限ありCCD IK
        static glm::quat limited_solve_ik(const glm::mat4& bone_global,
            const glm::mat4& ik_bone_global,
            const glm::mat4& target_bone_global,
            const glm::vec3& twist_axis,
            const float ik_limit);

        // スイング・ツイスト分解でクオータニオンを分解してから制限をかける
        static glm::quat decompose_swing_twist(const glm::quat& q,
            const glm::vec3& twist_axis,
            const float twist_min,
            const float twist_max,
            const float swing_max);

        static glm::quat clamp_swing_cone(
            const glm::quat& swing, const glm::vec3& twist_axis, const float max);

        static glm::quat quaternion_from_to(const glm::quat& from, const glm::quat& to);
    };
} // namespace enishi::ik