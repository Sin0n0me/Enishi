#include "opengl40_render_initializer.h"
#include "opengl40_helpers.h"

namespace enishi::renderer::opengl {
    foundation::Result<std::shared_ptr<OpenGL40Renderer>, platform::RenderError>
    OpenGL40RenderInitializer::init(std::shared_ptr<platform::IOpenGLContext> context) {
        if (!context || !context->make_current()) {
            return foundation::Error(
                platform::RenderError::MakeError, "Failed to make the OpenGL context current");
        }
        if (!helpers::load_gl40_functions(*context)) {
            return foundation::Error(
                platform::RenderError::MakeError, "OpenGL 4.0 is not available");
        }
        return std::make_shared<OpenGL40Renderer>(std::move(context));
    }
} // namespace enishi::renderer::opengl
