#pragma once
#include <animation/procedural/cursor/cursor_pose_state.h>
#include <animation/rig/bone_name_dictionary.h>
#include <component/model_component.h>
#include <utility>

namespace enishi::animation {
    class CursorReach {
      public:
        static void initialize_arms(CursorPoseState& state,
            const component::ModelComponent& model,
            const BoneNameDictionary& bone_names = BoneNameDictionary{});
        [[nodiscard]] static bool can_reach(
            const CursorPoseState& state, std::size_t side, const glm::vec3& target);
        [[nodiscard]] static CursorArmFrame arm_frame(
            const CursorPoseState& state, std::size_t side);
        static void solve_arm(CursorPoseState& state,
            const component::ModelComponent& model,
            component::AnimationComponent& animation,
            std::size_t side,
            const glm::vec3& target,
            const CursorArmFrame& frame);
        static void limit_arm_speed(CursorPoseState& state,
            const component::ModelComponent& model,
            component::AnimationComponent& animation,
            const std::array<glm::quat, std::to_underlying(CursorPoseState::Joint::Count)>&
                previous,
            float seconds);
        static void protect_arms(CursorPoseState& state,
            const component::ModelComponent& model,
            component::AnimationComponent& animation);
        static void reach(CursorPoseState& state,
            const component::ModelComponent& model,
            component::AnimationComponent& animation,
            const glm::vec3& target);

      private:
        [[nodiscard]] static bool arm_clear(
            const CursorPoseState& state, std::size_t side, const CursorArmFrame& frame);
        static void constrain_arm(const CursorPoseState& state,
            component::AnimationComponent& animation,
            std::size_t side);
        static bool reach_side(CursorPoseState& state,
            const component::ModelComponent& model,
            component::AnimationComponent& animation,
            std::size_t side,
            const glm::vec3& target);
    };
} // namespace enishi::animation
