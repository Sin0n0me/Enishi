#pragma once

#include "opengl40_renderer.h"
#include <platform/window/window_handle.h>

namespace enishi::renderer::opengl {
    class OpenGL40RenderInitializer {
      public:
        [[nodiscard]] foundation::Result<std::shared_ptr<OpenGL40Renderer>, platform::RenderError>
        init(const platform::WindowHandle& window, const types::WindowSize& window_size);
    };
} // namespace enishi::renderer::opengl
