#pragma once
#include <animation/procedural/cursor/cursor_pose_state.h>
#include <component/model_component.h>

namespace enishi::animation {
    class CursorIdle {
      public:
        static void apply_idle(CursorPoseState& state,
            const component::ModelComponent& model,
            component::AnimationComponent& animation,
            bool enabled,
            float seconds);
        static void apply_idle_arms(CursorPoseState& state,
            const component::ModelComponent& model,
            component::AnimationComponent& animation,
            float weight);

      private:
        static void apply_idle_legs(CursorPoseState& state,
            const component::ModelComponent& model,
            component::AnimationComponent& animation,
            float sway);
    };
} // namespace enishi::animation
