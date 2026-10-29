#pragma once
#include "interface_effect.h"
#include <memory>

namespace enishi::effect {
    // Main-thread API. The player must outlive every call and all effect callbacks.
    class IEffectPlayer {
      public:
        virtual ~IEffectPlayer() noexcept;
        // Each accepted instance starts once. Rejected instances are destroyed without starting.
        [[nodiscard]] virtual foundation::Result<EffectId, EffectError> play(
            std::unique_ptr<IEffect> effect) = 0;
        // Returns false for invalid, finished or unknown IDs. Mutations during callbacks return
        // Busy.
        [[nodiscard]] virtual foundation::Result<bool, EffectError> stop(EffectId id) = 0;
        [[nodiscard]] virtual foundation::Result<void, EffectError> stop_all() = 0;
        [[nodiscard]] virtual bool is_playing(EffectId id) const noexcept = 0;
    };
} // namespace enishi::effect
