#pragma once
#include <animation/procedural/cursor/cursor_pose_state.h>
#include <component/model_component.h>

namespace enishi::animation {
    class CursorCollision {
      public:
        static void initialize_collision(
            CursorPoseState& state, const component::ModelComponent& model);
        [[nodiscard]] static float hand_radius(const CursorPoseState& state, std::size_t side);
        [[nodiscard]] static bool mesh_arm_clear(const CursorPoseState& state, std::size_t side);

      private:
        static void initialize_collision_queries(
            CursorPoseState& state, const component::ModelComponent& model);
        [[nodiscard]] static const CursorCollisionBone& collision_bound(
            const CursorPoseState& state, std::size_t index);
    };
} // namespace enishi::animation
