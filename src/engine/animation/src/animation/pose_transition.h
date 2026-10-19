#pragma once
#include <component/animation_component.h>
#include <span>

namespace enishi::animation {
    class PoseTransition {
      private:
        std::vector<component::AnimationBuffer> source;
        std::vector<component::AnimationBuffer> destination;
        double elapsed = 0;
        bool running = false;

      public:
        static constexpr double DURATION = 0.5;
        void begin(std::span<const component::AnimationBuffer> from,
            std::span<const component::AnimationBuffer> to);
        void apply(float seconds, std::vector<component::AnimationBuffer>& output);
        void cancel();
        [[nodiscard]] bool active() const;
    };
} // namespace enishi::animation
