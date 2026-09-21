#include "opengl40_render_initializer.h"

#include <glad/gl.h>

namespace enishi::renderer::opengl {
    namespace {
        thread_local platform::IOpenGLContext* current_context = nullptr;

        GLADapiproc get_proc_address(const char* const name) {
            return current_context
                       ? reinterpret_cast<GLADapiproc>(current_context->get_proc_address(name))
                       : nullptr;
        }
    } // namespace

    foundation::Result<std::shared_ptr<OpenGL40Renderer>, platform::RenderError>
    OpenGL40RenderInitializer::init(std::shared_ptr<platform::IOpenGLContext> context) {
        if (!context || !context->make_current()) {
            return foundation::Error(
                platform::RenderError::MakeError, "Failed to make the OpenGL context current");
        }
        current_context = context.get();
        const auto loaded = gladLoadGL(get_proc_address);
        current_context = nullptr;
        if (!loaded || !GLAD_GL_VERSION_4_0) {
            return foundation::Error(
                platform::RenderError::MakeError, "OpenGL 4.0 is not available");
        }
        return std::make_shared<OpenGL40Renderer>(std::move(context));
    }
} // namespace enishi::renderer::opengl
