#pragma once
#include "crack_settings.h"
#include <effect/effect_types.h>
#include <engine_types/assets/texture/texture_data.h>
namespace enishi::effect {
    [[nodiscard]] foundation::Result<void, EffectError> validate_crack_settings(
        const CrackSettings& settings);
    // R: cavity depth (0 = unbroken surface), G: crack coverage. Distances are in cell units.
    [[nodiscard]] foundation::Result<types::TextureData, EffectError> make_crack_mask(
        const CrackSettings& settings);
} // namespace enishi::effect
