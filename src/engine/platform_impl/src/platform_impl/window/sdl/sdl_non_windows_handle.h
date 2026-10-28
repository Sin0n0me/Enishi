#pragma once
#include <SDL3/SDL.h>
#include <foundation/option/option.h>
#include <platform/window/window_handle.h>
namespace enishi::platform_impl {
    inline foundation::Option<platform::NativeWindowsHandle> read_windows_handle(SDL_Window*) {
        // GL and Vulkan use the SDL window/context, not the Windows transport handle.
        return platform::NativeWindowsHandle{};
    }
} // namespace enishi::platform_impl
