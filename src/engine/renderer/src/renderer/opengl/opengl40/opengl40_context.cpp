#include "opengl40_context.h"
#include <glad/gl.h>

namespace enishi::renderer::opengl {
    OpenGL40Context::OpenGL40Context(SDL_Window* const window, SDL_GLContext const context) noexcept
        : context(context), window(window) {}

    OpenGL40Context::~OpenGL40Context(void) noexcept {
        if (this->context) {
            SDL_GL_DestroyContext(this->context);
        }
    }

    foundation::Result<std::shared_ptr<OpenGL40Context>, platform::RenderError>
    OpenGL40Context::make(SDL_Window* const window) {
        if (!window) {
            return foundation::Error(platform::RenderError::MakeError, "OpenGL window is null");
        }

        const auto context = SDL_GL_CreateContext(window);
        if (!context) {
            return foundation::Error(platform::RenderError::MakeError, SDL_GetError());
        }
        auto result = std::shared_ptr<OpenGL40Context>(new OpenGL40Context(window, context));
        if (!result->make_current()) {
            return foundation::Error(platform::RenderError::MakeError, SDL_GetError());
        }
        if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress))) {
            return foundation::Error(platform::RenderError::MakeError, "OpenGL function loading failed");
        }
        if (!GLAD_GL_VERSION_4_0) {
            return foundation::Error(platform::RenderError::MakeError, "OpenGL 4.0 is not available");
        }
        SDL_GL_SetSwapInterval(1);
        return result;
    }

    SDL_Window* OpenGL40Context::get_window(void) const noexcept { return this->window; }
    bool OpenGL40Context::make_current(void) const noexcept {
        return SDL_GL_MakeCurrent(this->window, this->context);
    }
    void OpenGL40Context::present(void) const noexcept { SDL_GL_SwapWindow(this->window); }
} // namespace enishi::renderer::opengl
