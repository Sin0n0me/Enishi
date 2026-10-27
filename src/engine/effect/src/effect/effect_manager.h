#pragma once
#include "interface_effect_player.h"
#include <map>
#include <vector>

namespace enishi::tests {
    struct EffectManagerTestAccess;
}
namespace enishi::effect {
    class EffectManager final : public IEffectPlayer {
      private:
        struct Entry;
        std::map<EffectId::Value, std::unique_ptr<Entry>> entries;
        EffectId::Value next_id = 1;
        bool busy = false;
        friend struct enishi::tests::EffectManagerTestAccess;

      public:
        EffectManager();
        ~EffectManager() noexcept override;
        EffectManager(const EffectManager&) = delete;
        EffectManager& operator=(const EffectManager&) = delete;
        [[nodiscard]] foundation::Result<EffectId, EffectError> play(
            std::unique_ptr<IEffect> effect) override;
        [[nodiscard]] foundation::Result<bool, EffectError> stop(EffectId id) override;
        [[nodiscard]] foundation::Result<void, EffectError> stop_all() override;
        [[nodiscard]] bool is_playing(EffectId id) const noexcept override;
        // Updates in creation order. Failed instances stop; their diagnostics are returned
        // together.
        [[nodiscard]] foundation::Result<std::vector<EffectFailure>, EffectError> update(
            const types::DeltaTime& delta_time);
    };
} // namespace enishi::effect
