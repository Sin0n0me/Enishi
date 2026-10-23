#include "opengl40_errors.h"
#include <glad/gl.h>
namespace enishi::renderer::opengl {
    platform::RenderResult<void> check_gl_errors(const char* operation) {
        auto code = glGetError();
        if (code == GL_NO_ERROR) {
            return {};
        }
        auto error = foundation::Error(platform::RenderError::MakeError,
            std::format("OpenGL operation failed: {}", operation));
        do {
            error.add_message(std::format("OpenGL error {:#x}", code));
            code = glGetError();
        } while (code != GL_NO_ERROR);
        return error;
    }
} // namespace enishi::renderer::opengl
