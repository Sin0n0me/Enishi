#pragma once
#include "effect_types.h"
#include <engine_types/system/delta_time.h>

namespace enishi::effect {
    class IEffect {
      private:
        bool managed = false;
        friend class EffectManager;

      public:
        inline IEffect() = default;
        IEffect(const IEffect&) = delete;
        IEffect& operator=(const IEffect&) = delete;
        virtual ~IEffect() noexcept = default;
        [[nodiscard]] virtual foundation::Result<void, EffectError> start() = 0;
        [[nodiscard]] virtual foundation::Result<EffectStatus, EffectError> update(
            const types::DeltaTime& delta_time) = 0;
        // Called exactly once after a start attempt, including failed starts. Must not throw.
        // Implementations own resources through RAII and may release partial initialization here.
        virtual void stop() noexcept = 0;
    };
} // namespace enishi::effect
