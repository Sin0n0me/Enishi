#include "sdl_non_windows_handle.h"
namespace enishi::platform_impl {
    foundation::Option<platform::NativeWindowsHandle> read_windows_handle(SDL_Window*) {
        // GL and Vulkan use the SDL window/context, not the Windows transport handle.
        return platform::NativeWindowsHandle{};
    }
} // namespace enishi::platform_impl
