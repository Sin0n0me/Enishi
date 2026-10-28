#include "effect_overlay_linux.h"
#include <SDL3/SDL.h>
#include <X11/Xlib.h>
#include <memory>
#include <string_view>
namespace enishi::platform_impl {
    bool prepare_effect_overlay(SDL_Window* window) {
        const char* driver = SDL_GetCurrentVideoDriver();
        if (driver == nullptr || std::string_view{driver} != "x11") {
            return SDL_SetError(
                "Desktop effect overlays require X11; Wayland global positioning is unavailable");
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
    bool set_effect_overlay_layer(SDL_Window* window, effect::EffectLayer layer) {
        if (layer != effect::EffectLayer::Foreground && layer != effect::EffectLayer::Background) {
            return SDL_SetError("Unknown effect layer");
        }
        const bool foreground = layer == effect::EffectLayer::Foreground;
        if (!SDL_SetWindowAlwaysOnTop(window, foreground)) {
            return false;
        }
        if (!foreground) {
            return true;
        }
        // SDL_RaiseWindow can request activation. Use XRaiseWindow directly to preserve focus.
        // SDL's X11 backend already requires this library; no additional linked dependency.
        static const std::unique_ptr<SDL_SharedObject, decltype(&SDL_UnloadObject)> library{
            SDL_LoadObject("libX11.so.6"), SDL_UnloadObject};
        if (library == nullptr) {
            return SDL_SetError("Cannot load the X11 library for effect stacking");
        }
        static const auto raise_window = reinterpret_cast<decltype(&XRaiseWindow)>(
            SDL_LoadFunction(library.get(), "XRaiseWindow"));
        static const auto flush =
            reinterpret_cast<decltype(&XFlush)>(SDL_LoadFunction(library.get(), "XFlush"));
        if (raise_window == nullptr || flush == nullptr) {
            return SDL_SetError("X11 effect stacking functions are unavailable");
        }
        const auto properties = SDL_GetWindowProperties(window);
        auto* display = static_cast<Display*>(
            SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr));
        const auto native = static_cast<Window>(
            SDL_GetNumberProperty(properties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
        if (display == nullptr || native == 0) {
            return SDL_SetError("Overlay X11 handles are unavailable");
        }
        raise_window(display, native);
        flush(display);
        return true;
    }
} // namespace enishi::platform_impl
