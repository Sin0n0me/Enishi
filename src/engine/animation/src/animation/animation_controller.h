#pragma once
#include "interface_animation_controller.h"
#include <unordered_map>

namespace enishi::animation {
    class AnimationController final : public IAnimationController {
      private:
        std::unordered_map<std::string, std::shared_ptr<const AnimationClipData>> clips;
        std::shared_ptr<const AnimationClipData> active_clip;
        float time = 0.0f;
        bool playing = false;

      public:
        void add_clip(std::string name, std::shared_ptr<const AnimationClipData> clip) override;
        [[nodiscard]] bool play(const std::string& name, const bool restart = true) override;
        void pause(void) override;
        void stop(void) override;
        void update(const float elapsed_seconds) override;
        [[nodiscard]] const AnimationClipData* get_active_clip(void) const noexcept override;
        [[nodiscard]] float get_time(void) const noexcept override;
        [[nodiscard]] bool is_playing(void) const noexcept override;
    };
} // namespace enishi::animation
