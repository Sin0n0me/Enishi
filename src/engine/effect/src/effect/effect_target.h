#pragma once
#include <cstdint>
namespace enishi::effect {
    enum class EffectTargetKind { AllDisplays, DisplayAtPoint };
    struct EffectTarget {
        EffectTargetKind kind = EffectTargetKind::AllDisplays;
        std::int32_t x = 0;
        std::int32_t y = 0;

        [[nodiscard]] static constexpr EffectTarget all_displays() noexcept {
            return {};
        }
        // Desktop coordinates use the same logical units as window positions.
        [[nodiscard]] static constexpr EffectTarget display_at(
            std::int32_t x, std::int32_t y) noexcept {
            return {EffectTargetKind::DisplayAtPoint, x, y};
        }
    };
} // namespace enishi::effect
