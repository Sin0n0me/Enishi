#include "opengl40_renderer_state.h"
#include <glad/gl.h>
namespace enishi::renderer::opengl {
    namespace {
        constexpr GLuint NO_GL_OBJECT = 0;
    }
    OpenGL40RendererState::OpenGL40RendererState(void)
        : topology(GL_TRIANGLES)
        , active_vertex_shader(NO_GL_OBJECT)
        , active_fragment_shader(NO_GL_OBJECT)
        , active_program(NO_GL_OBJECT)
        , active_framebuffer(NO_GL_OBJECT)
        , active_index_type(GL_UNSIGNED_INT)
        , active_sampler(NO_GL_OBJECT)
        , back_buffer_framebuffer(NO_GL_OBJECT)
        , back_buffer_color(NO_GL_OBJECT)
        , back_buffer_width(0)
        , back_buffer_height(0)
        , uniform_buffer_binding_limit(0)
        , texture_unit_limit(0)
        , is_back_buffer_framebuffer_complete(false) {
    }

    void OpenGL40RendererState::initialize() {
        glEnable(GL_DEPTH_TEST);
        glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &this->uniform_buffer_binding_limit);
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &this->texture_unit_limit);
    }
    void OpenGL40RendererState::release() noexcept {
        glDeleteBuffers(static_cast<GLsizei>(this->buffers.size()), this->buffers.data());
        glDeleteTextures(static_cast<GLsizei>(this->textures.size()), this->textures.data());
        glDeleteVertexArrays(
            static_cast<GLsizei>(this->vertex_arrays.size()), this->vertex_arrays.data());
        glDeleteSamplers(static_cast<GLsizei>(this->gl_samplers.size()), this->gl_samplers.data());
        for (const auto shader : this->shaders) {
            glDeleteShader(shader);
        }
        for (const auto& [_, program] : this->programs) {
            glDeleteProgram(program);
        }
        glDeleteFramebuffers(
            static_cast<GLsizei>(this->framebuffers.size()), this->framebuffers.data());
    }
} // namespace enishi::renderer::opengl
