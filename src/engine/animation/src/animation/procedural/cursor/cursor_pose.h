#pragma once
#include "cursor_pose_state.h"
#include <component/model_component.h>

namespace enishi::animation {
    // Owns pose state and sequences the stateless procedural calculations.
    class CursorPose {
      private:
        using PoseStage = CursorTransitionState::Stage;
        CursorPoseState state;

      public:
        explicit CursorPose(const component::ModelComponent& model, CursorPoseConfig config = {});
        void apply_transitioned(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            const std::optional<CursorRay>& cursor,
            float seconds,
            bool idle_enabled);
        void apply(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            const std::optional<CursorRay>& cursor,
            float seconds,
            bool idle_enabled = false);
        void restore(component::AnimationComponent& animation);
        [[nodiscard]] std::optional<std::pair<std::size_t, float>> pick_hand(
            const component::AnimationComponent& animation, const CursorRay& cursor) const;
        void begin_hanging(std::size_t side,
            const component::AnimationComponent& animation,
            const CursorRay& cursor);
        void end_hanging();
        [[nodiscard]] bool is_hanging() const;
        [[nodiscard]] bool suppress_hanging_ik(types::BoneIndex bone) const;

      private:
        [[nodiscard]] PoseStage pose_stage(const component::AnimationComponent& animation) const;
        void remember_transition_pose(const component::AnimationComponent& animation);
    };
} // namespace enishi::animation
