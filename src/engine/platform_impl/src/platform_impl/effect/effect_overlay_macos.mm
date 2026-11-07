#include "effect_overlay_macos.h"
#import <AppKit/AppKit.h>
namespace enishi::platform_impl {
    bool prepare_effect_overlay(SDL_Window* window) {
        auto* native = static_cast<NSWindow*>(SDL_GetPointerProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr));
        if (native == nil) {
            return SDL_SetError("Overlay NSWindow is unavailable");
        }
        [native setIgnoresMouseEvents:YES];
        [native setOpaque:NO];
        [native setBackgroundColor:[NSColor clearColor]];
        [native setHasShadow:NO];
        return true;
    }
    bool set_effect_overlay_layer(SDL_Window* window, effect::EffectLayer layer) {
        if (layer != effect::EffectLayer::Foreground && layer != effect::EffectLayer::Background) {
            return SDL_SetError("Unknown effect layer");
        }
        auto* native = static_cast<NSWindow*>(SDL_GetPointerProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr));
        if (native == nil) {
            return SDL_SetError("Overlay NSWindow is unavailable");
        }
        const bool foreground = layer == effect::EffectLayer::Foreground;
        if (!SDL_SetWindowAlwaysOnTop(window, foreground)) {
            return false;
        }
        // SDL's always-on-top model windows use NSFloatingWindowLevel. The next level
        // keeps an input-transparent overlay above them without making it the key window.
        constexpr NSInteger FOREGROUND_LEVEL_OFFSET = 1;
        [native setLevel:foreground ? NSFloatingWindowLevel + FOREGROUND_LEVEL_OFFSET
                                    : NSNormalWindowLevel];
        if ((SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN) == 0) {
            [native orderWindow:NSWindowAbove relativeTo:0];
        }
        return true;
    }
} // namespace enishi::platform_impl
