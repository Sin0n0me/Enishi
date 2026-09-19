#pragma once

#include "opengl40_renderer.h"
#include <SDL3/SDL.h>

namespace enishi::renderer::opengl {
    class OpenGL40RenderInitializer {
      public:
        [[nodiscard]] foundation::Result<std::shared_ptr<OpenGL40Renderer>, platform::RenderError>
        init(SDL_Window* window, const types::WindowSize& window_size);
    };
} // namespace enishi::renderer::opengl
