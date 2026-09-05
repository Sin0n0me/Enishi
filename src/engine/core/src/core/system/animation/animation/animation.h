#pragma once
#include <component/animation_component.h>

namespace enishi::core {
    // ロジックのみ
    class Animation {
      public:
        static void apply_animation(
            component::AnimationComponent& animation, const float elapsed_time);
    };
} // namespace enishi::core