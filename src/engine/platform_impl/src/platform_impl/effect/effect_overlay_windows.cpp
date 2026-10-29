#include "effect_overlay_windows.h"
#include <SDL3/SDL.h>
#include <Windows.h>
namespace enishi::platform_impl {
    bool prepare_effect_overlay(SDL_Window* window) {
        const auto native = static_cast<HWND>(SDL_GetPointerProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
        if (native == nullptr) {
            return SDL_SetError("Overlay HWND is unavailable");
        }
        SetLastError(ERROR_SUCCESS);
        const auto style = GetWindowLongPtrW(native, GWL_EXSTYLE);
        if (style == 0 && GetLastError() != ERROR_SUCCESS) {
            return SDL_SetError("Cannot read overlay window style");
        }
        SetLastError(ERROR_SUCCESS);
        if (SetWindowLongPtrW(native,
                GWL_EXSTYLE,
                style | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW) ==
                0 &&
            GetLastError() != ERROR_SUCCESS) {
            return SDL_SetError("Cannot make overlay input transparent");
        }
        // Layered + transparent passes hit testing through to windows in other processes.
        constexpr BYTE FULL_OPACITY = 255;
        if (SetLayeredWindowAttributes(native, 0, FULL_OPACITY, LWA_ALPHA) == 0) {
            return SDL_SetError("Cannot initialize layered overlay opacity");
        }
        return true;
    }
    bool set_effect_overlay_layer(SDL_Window* window, effect::EffectLayer layer) {
        if (layer != effect::EffectLayer::Foreground && layer != effect::EffectLayer::Background) {
            return SDL_SetError("Unknown effect layer");
        }
        const bool foreground = layer == effect::EffectLayer::Foreground;
        if (!SDL_SetWindowAlwaysOnTop(window, foreground)) {
            return false;
        }
        const auto native = static_cast<HWND>(SDL_GetPointerProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
        if (native == nullptr) {
            return SDL_SetError("Overlay HWND is unavailable");
        }
        // ALWAYS_ON_TOP alone does not order two topmost windows. Reassert the overlay's
        // position after model clicks/activation without transferring keyboard focus.
        if (SetWindowPos(native,
                foreground ? HWND_TOPMOST : HWND_NOTOPMOST,
                0,
                0,
                0,
                0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER) == 0) {
            return SDL_SetError("Cannot update effect overlay stacking: %lu", GetLastError());
        }
        return true;
    }
} // namespace enishi::platform_impl
