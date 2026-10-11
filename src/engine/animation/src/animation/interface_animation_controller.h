#pragma once
#include "clip_data/animation.h"
#include <memory>
#include <string>

namespace enishi::animation {
    class IAnimationController {
      public:
        virtual ~IAnimationController(void) noexcept = default;
        virtual void add_clip(std::string name, std::shared_ptr<const AnimationClipData> clip) = 0;
        [[nodiscard]] virtual bool play(const std::string& name, const bool restart = true) = 0;
        virtual void pause(void) = 0;
        virtual void stop(void) = 0;
        virtual void update(const float elapsed_seconds) = 0;
        [[nodiscard]] virtual const AnimationClipData* get_active_clip(void) const noexcept = 0;
        [[nodiscard]] virtual float get_time(void) const noexcept = 0;
        [[nodiscard]] virtual bool is_playing(void) const noexcept = 0;
    };
} // namespace enishi::animation
