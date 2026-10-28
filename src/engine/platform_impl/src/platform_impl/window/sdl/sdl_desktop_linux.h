#pragma once
#include <SDL3/SDL.h>
namespace enishi::platform_impl {
    // Desktop placement and global cursor tracking require X11, including on XWayland.
    constexpr const char* DESKTOP_VIDEO_DRIVER = "x11";
    constexpr int OPENGL_MINOR_VERSION = 0;
    constexpr int OPENGL_CONTEXT_FLAGS = 0;
} // namespace enishi::platform_impl
