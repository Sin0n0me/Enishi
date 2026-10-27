#pragma once
#include <SDL3/SDL.h>
#include <memory>
#include <string_view>
namespace enishi::platform_impl {
    inline bool prepare_crack_overlay(SDL_Window* window) {
        const char* driver = SDL_GetCurrentVideoDriver();
        if (driver == nullptr || std::string_view{driver} != "x11") {
            return SDL_SetError(
                "Desktop crack overlays require X11; Wayland global positioning is unavailable");
        }
        constexpr int EMPTY_MASK_SIZE = 1;
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> shape{
            SDL_CreateSurface(EMPTY_MASK_SIZE, EMPTY_MASK_SIZE, SDL_PIXELFORMAT_RGBA32),
            SDL_DestroySurface};
        if (shape == nullptr) {
            return false;
        }
        // X11's SDL shape affects ShapeInput only: all-zero alpha passes every pointer event
        // through.
        if (!SDL_ClearSurface(shape.get(), 0, 0, 0, 0)) {
            return false;
        }
        return SDL_SetWindowShape(window, shape.get());
    }
} // namespace enishi::platform_impl
