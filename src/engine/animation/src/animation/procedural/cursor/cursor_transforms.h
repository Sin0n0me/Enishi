#pragma once
#include <animation/procedural/cursor/cursor_pose_state.h>
#include <component/model_component.h>

namespace enishi::animation {
    class CursorTransforms {
      public:
        static void update_globals(CursorTransformCache& transforms,
            const component::ModelComponent& model,
            const component::AnimationComponent& animation);
    };
} // namespace enishi::animation
