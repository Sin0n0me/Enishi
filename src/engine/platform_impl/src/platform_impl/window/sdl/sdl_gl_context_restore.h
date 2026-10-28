#pragma once
#include <SDL3/SDL.h>
#include <debug/log/logger.h>
#include <string>
namespace enishi::platform_impl {
    class SDLGLContextRestore {
      private:
        SDL_Window* window = SDL_GL_GetCurrentWindow();
        SDL_GLContext context = SDL_GL_GetCurrentContext();
        bool enabled;

      public:
        inline explicit SDLGLContextRestore(bool enabled = true)
            : enabled(enabled) {
        }
        SDLGLContextRestore(const SDLGLContextRestore&) = delete;
        SDLGLContextRestore& operator=(const SDLGLContextRestore&) = delete;
        inline ~SDLGLContextRestore() {
            if (this->enabled &&
                (SDL_GL_GetCurrentContext() != this->context ||
                    SDL_GL_GetCurrentWindow() != this->window) &&
                !SDL_GL_MakeCurrent(this->window, this->context)) {
                debug::Logger::warning(
                    std::string{"Failed to restore OpenGL context: "} + SDL_GetError());
            }
        }
    };
} // namespace enishi::platform_impl
