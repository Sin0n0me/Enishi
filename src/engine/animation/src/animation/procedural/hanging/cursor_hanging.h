#pragma once
#include <animation/procedural/cursor/cursor_pose_state.h>
#include <component/model_component.h>

namespace enishi::animation {
    class CursorHanging {
      private:
        static constexpr float HANGING_DROP_SECONDS = 0.3f;
        static constexpr float HANGING_LANDING_SECONDS = 0.3f;

      public:
        [[nodiscard]] static std::optional<std::pair<std::size_t, float>> pick_hand(
            const CursorPoseState& state,
            const component::AnimationComponent& animation,
            const CursorRay& cursor);
        static void begin_hanging(CursorPoseState& state,
            std::size_t side,
            const component::AnimationComponent& animation,
            const CursorRay& cursor);
        static void end_hanging(CursorPoseState& state);
        [[nodiscard]] static bool is_hanging(const CursorPoseState& state);
        [[nodiscard]] static bool suppress_hanging_ik(
            const CursorPoseState& state, types::BoneIndex bone);
        static void update_hanging_ik_targets(CursorPoseState& state,
            const component::ModelComponent& model,
            component::AnimationComponent& animation);
        static void translate_hanging_roots(const CursorPoseState& state,
            component::AnimationComponent& animation,
            const glm::vec3& offset);
        static bool apply_hanging(CursorPoseState& state,
            const component::ModelComponent& model,
            component::AnimationComponent& animation,
            const std::optional<CursorRay>& cursor,
            float seconds);

      private:
        static void prepare_hanging_limbs(CursorPoseState& state,
            const component::ModelComponent& model,
            component::AnimationComponent& animation);
        static void hang_limb(CursorPoseState& state,
            const component::ModelComponent& model,
            component::AnimationComponent& animation,
            types::BoneIndex start,
            types::BoneIndex end,
            const glm::vec3& target_direction);
        static void blend_hanging_pose(
            const CursorPoseState& state, component::AnimationComponent& animation);
        static void apply_hanging_walk(CursorPoseState& state,
            const component::ModelComponent& model,
            component::AnimationComponent& animation,
            float seconds);
        static bool apply_hanging_landing(CursorPoseState& state,
            const component::ModelComponent& model,
            component::AnimationComponent& animation,
            float seconds);
    };
} // namespace enishi::animation
