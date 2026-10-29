#pragma once
#include <SDL3/SDL.h>
#include <effect/effect_target.h>
#include <effect/effect_types.h>
#include <optional>
namespace enishi::platform_impl {
    // No display ID means all displays. Resolution uses the current desktop layout.
    [[nodiscard]] foundation::Result<std::optional<SDL_DisplayID>, effect::EffectError>
    resolve_effect_display(const effect::EffectTarget& target);
} // namespace enishi::platform_impl
