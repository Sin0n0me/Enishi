#pragma once

#include <SDL3/SDL.h>
#include <foundation/result/result.h>
#include <memory>
#include <platform/errors/renderer_errors.h>

namespace enishi::renderer::opengl {
    class OpenGL40Context {
      private:
        SDL_GLContext context = nullptr;
        SDL_Window* window = nullptr;

        explicit OpenGL40Context(SDL_Window* window, SDL_GLContext context) noexcept;

      public:
        ~OpenGL40Context(void) noexcept;

        OpenGL40Context(const OpenGL40Context&) = delete;
        OpenGL40Context& operator=(const OpenGL40Context&) = delete;

        [[nodiscard]] static foundation::Result<std::shared_ptr<OpenGL40Context>, platform::RenderError>
        make(SDL_Window* window);

        [[nodiscard]] SDL_Window* get_window(void) const noexcept;
        [[nodiscard]] bool make_current(void) const noexcept;
        void present(void) const noexcept;
    };
} // namespace enishi::renderer::opengl
