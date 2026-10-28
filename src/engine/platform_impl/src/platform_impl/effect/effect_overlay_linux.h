#pragma once
#include <SDL3/SDL.h>
namespace enishi::platform_impl {
    [[nodiscard]] bool prepare_effect_overlay(SDL_Window* window);
}
