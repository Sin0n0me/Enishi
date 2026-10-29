#include "sdl_gl_context_restore.h"
#include <debug/log/logger.h>
#include <string>
namespace enishi::platform_impl {
    SDLGLContextRestore::SDLGLContextRestore(bool enabled)
        : window(SDL_GL_GetCurrentWindow())
        , context(SDL_GL_GetCurrentContext())
        , enabled(enabled) {
    }
    SDLGLContextRestore::~SDLGLContextRestore() {
        if (this->enabled &&
            (SDL_GL_GetCurrentContext() != this->context ||
                SDL_GL_GetCurrentWindow() != this->window) &&
            !SDL_GL_MakeCurrent(this->window, this->context)) {
            debug::Logger::warning(
                std::string{"Failed to restore OpenGL context: "} + SDL_GetError());
        }
    }
} // namespace enishi::platform_impl
