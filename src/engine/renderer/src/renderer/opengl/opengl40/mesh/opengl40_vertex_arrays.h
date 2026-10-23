#pragma once
#include <glad/gl.h>
#include <renderer/opengl/opengl40/runtime/opengl40_renderer_state.h>

namespace enishi::renderer::opengl {
    class OpenGL40VertexArrays final {
      public:
        static platform::RenderResult<GLuint> create(
            OpenGL40RendererState& runtime, GLuint vertex, GLuint index, GLsizei stride);
    };
} // namespace enishi::renderer::opengl
