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

      public:
        CrackEffect(CrackSettings settings, std::unique_ptr<ICrackSurface> surface);
        ~CrackEffect() noexcept override;
        foundation::Result<void, EffectError> start() override;
        foundation::Result<EffectStatus, EffectError> update(
            const types::DeltaTime& delta_time) override;
        void stop() noexcept override;
    };
} // namespace enishi::effect
