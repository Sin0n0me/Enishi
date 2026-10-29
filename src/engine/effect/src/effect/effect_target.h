#pragma once
#include <cstdint>
namespace enishi::effect {
    // Model windows occupy the always-on-top layer. Foreground is raised above it;
    // background uses the normal desktop window layer below it.
    enum class EffectLayer { Foreground, Background };
    enum class EffectTargetKind { AllDisplays, DisplayAtPoint };
    struct EffectTarget {
        EffectTargetKind kind = EffectTargetKind::AllDisplays;
        std::int32_t x = 0;
        std::int32_t y = 0;
        EffectLayer layer = EffectLayer::Foreground;

        [[nodiscard]] static constexpr EffectTarget all_displays(
            EffectLayer layer = EffectLayer::Foreground) noexcept {
            return {EffectTargetKind::AllDisplays, 0, 0, layer};
        }
        // Desktop coordinates use the same logical units as window positions.
        [[nodiscard]] static constexpr EffectTarget display_at(
            std::int32_t x, std::int32_t y, EffectLayer layer = EffectLayer::Foreground) noexcept {
            return {EffectTargetKind::DisplayAtPoint, x, y, layer};
        }
    };
} // namespace enishi::effect
