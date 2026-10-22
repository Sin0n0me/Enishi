#pragma once
#include <animation/procedural/cursor/cursor_pose_state.h>
#include <component/model_component.h>

namespace enishi::animation {
    class CursorGaze {
      public:
        [[nodiscard]] static std::optional<glm::vec3> target(
            const CursorPoseState& state, const CursorRay& cursor);
        [[nodiscard]] static std::optional<glm::vec3> gaze_offset(
            const CursorPoseState& state, const CursorRay& cursor);
        static void look(const CursorPoseState& state,
            component::AnimationComponent& animation,
            CursorPoseState::Joint joint,
            const glm::vec3& offset,
            float yaw_limit,
            float pitch_limit);
    };
} // namespace enishi::animation
