#pragma once
#include <animation/procedural/cursor/cursor_pose_state.h>
#include <component/model_component.h>

namespace enishi::animation {
    class CursorJump {
      public:
        [[nodiscard]] static bool cursor_in_arm_reach(
            const CursorPoseState& state, std::size_t side);
        [[nodiscard]] static float desired_jump_height(
            const CursorPoseState& state, const glm::vec3& point);
        static void apply_jump(CursorPoseState& state,
            const component::ModelComponent& model,
            component::AnimationComponent& animation,
            const std::optional<glm::vec3>& point,
            float seconds);
    };
} // namespace enishi::animation
