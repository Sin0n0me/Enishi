#pragma once
#include <SDL3/SDL.h>
namespace enishi::platform_impl {
    constexpr const char* DESKTOP_VIDEO_DRIVER = "cocoa";
    constexpr int OPENGL_MINOR_VERSION = 1;
    constexpr int OPENGL_CONTEXT_FLAGS = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;
} // namespace enishi::platform_impl
