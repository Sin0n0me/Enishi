#pragma once
#include <SDL3/SDL.h>
#include <effect/effect_target.h>
namespace enishi::platform_impl {
    [[nodiscard]] bool prepare_effect_overlay(SDL_Window* window);
    [[nodiscard]] bool set_effect_overlay_layer(SDL_Window* window, effect::EffectLayer layer);
} // namespace enishi::platform_impl
