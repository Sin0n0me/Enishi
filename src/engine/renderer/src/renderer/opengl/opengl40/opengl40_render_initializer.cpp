#include "opengl40_render_initializer.h"
#include "opengl40_helpers.h"
#include <cstddef>
#include <engine_types/renderer/uniform_buffer/bones.h>
#include <glad/gl.h>

namespace enishi::renderer::opengl {
    constexpr std::size_t MODEL_BONES_UNIFORM_BLOCK_SIZE = sizeof(types::MediumModelBones);

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
        GLint max_uniform_block_size = 0;
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &max_uniform_block_size);
        if (max_uniform_block_size < 0 ||
            static_cast<std::size_t>(max_uniform_block_size) < MODEL_BONES_UNIFORM_BLOCK_SIZE) {
            return foundation::Error(platform::RenderError::MakeError,
                "OpenGL uniform block size is too small for the model bone matrices");
        }
        return std::make_shared<OpenGL40Renderer>(std::move(context));
    }
} // namespace enishi::renderer::opengl
