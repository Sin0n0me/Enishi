#pragma once
#include <SDL3/SDL.h>
#include <foundation/option/option.h>
#include <platform/window/window_handle.h>
namespace enishi::platform_impl {
    [[nodiscard]] foundation::Option<platform::NativeWindowsHandle> read_windows_handle(
        SDL_Window* window);
} // namespace enishi::platform_impl
