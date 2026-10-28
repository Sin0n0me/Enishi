#pragma once
#include <SDL3/SDL.h>
#include <foundation/option/option.h>
#include <platform/window/window_handle.h>
namespace enishi::platform_impl {
    inline foundation::Option<platform::NativeWindowsHandle> read_windows_handle(
        SDL_Window* window) {
        const auto properties = SDL_GetWindowProperties(window);
        const auto hwnd =
            SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        const auto instance =
            SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, nullptr);
        if (hwnd == nullptr || instance == nullptr) {
            return {};
        }
        return platform::NativeWindowsHandle{hwnd, instance};
    }
} // namespace enishi::platform_impl
