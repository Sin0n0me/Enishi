#pragma once
#include <animation/procedural/cursor/cursor_pose_state.h>
#include <animation/rig/bone_name_dictionary.h>
#include <component/model_component.h>

namespace enishi::animation {
    class CursorLegs {
      public:
        static void initialize_legs(CursorPoseState& state,
            const component::ModelComponent& model,
            const BoneNameDictionary& bone_names = BoneNameDictionary{});
        static void bend_leg(CursorPoseState& state,
            const component::ModelComponent& model,
            component::AnimationComponent& animation,
            std::size_t side,
            const glm::mat4& ankle);
        static void crouch(CursorPoseState& state,
            const component::ModelComponent& model,
            component::AnimationComponent& animation,
            float weight);
    };
} // namespace enishi::animation
