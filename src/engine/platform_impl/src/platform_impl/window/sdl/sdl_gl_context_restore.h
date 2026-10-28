#pragma once
#include <SDL3/SDL.h>
namespace enishi::platform_impl {
    class SDLGLContextRestore {
      private:
        SDL_Window* window;
        SDL_GLContext context;
        bool enabled;

      public:
        explicit SDLGLContextRestore(bool enabled = true);
        SDLGLContextRestore(const SDLGLContextRestore&) = delete;
        ~SDLGLContextRestore();
        SDLGLContextRestore& operator=(const SDLGLContextRestore&) = delete;
    };
} // namespace enishi::platform_impl
