#pragma once
#include <component/animation_component.h>
#include <component/ik_component.h>
#include <component/model_component.h>
#include <engine_types/assets/model/addons/bone.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace enishi::core {
    // 純粋なロジックのみ
    // 状態などを持たない
    class IKSolver {
      public:
        static void apply_ik(component::AnimationComponent& animation,
            const component::IKComponent& ik_component,
            const component::ModelComponent& model,
            const types::BoneIndex index);

      private:
        static void ccd_ik(component::AnimationComponent& animation,
            const component::ModelComponent& model,
            const component::IK& ik_info,
            const types::BoneIndex index,
            const bool is_limited);

        // 制限なしCCD IK
        static glm::quat solve_ik(const component::AnimationBuffer& bone_node,
            const component::AnimationBuffer& ik_bone_node,
            const component::AnimationBuffer& target_bone_node,
            const float ik_limit);

        // 制限ありCCD IK
        static glm::quat limited_solve_ik(const component::AnimationBuffer& bone_node,
            const component::AnimationBuffer& ik_bone_node,
            const component::AnimationBuffer& target_bone_node,
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
} // namespace enishi::core