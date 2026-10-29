#pragma once
#include "interface_crack_surface.h"
#include <effect/interface_effect.h>
#include <memory>
namespace enishi::effect {
    class CrackEffect final : public IEffect {
      private:
        CrackSettings settings;
        std::unique_ptr<ICrackSurface> surface;
        double elapsed_seconds = 0;
        bool started = false;
        bool resources_open = false;
        bool reusable = false;
        std::optional<CrackImpact> impact;
        EffectTarget target;

      public:
        CrackEffect(CrackSettings settings, std::unique_ptr<ICrackSurface> surface);
        ~CrackEffect() noexcept override;
        // Prepare on the render thread before interactive playback; stop retains these resources.
        foundation::Result<void, EffectError> prepare();
        foundation::Result<void, EffectError> start() override;
        foundation::Result<EffectStatus, EffectError> update(
            const types::DeltaTime& delta_time) override;
        void stop() noexcept override;
        void set_impact(std::optional<CrackImpact> origin) noexcept;
        void set_target(const EffectTarget& destination) noexcept;

      private:
        foundation::Result<void, EffectError> open();
        void release() noexcept;
    };
} // namespace enishi::effect
