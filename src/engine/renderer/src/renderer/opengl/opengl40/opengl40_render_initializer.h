#pragma once

#include "opengl40_renderer.h"

namespace enishi::renderer::opengl {
    class OpenGL40RenderInitializer {
      public:
        [[nodiscard]] foundation::Result<std::shared_ptr<OpenGL40Renderer>, platform::RenderError>
        init(std::shared_ptr<platform::IOpenGLContext> context);
    };
} // namespace enishi::renderer::opengl
