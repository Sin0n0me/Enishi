#include "opengl40_buffers.h"
#include <glad/gl.h>
#include <limits>
#include <renderer/opengl/opengl40/runtime/opengl40_handles.h>
#include <renderer/opengl/opengl40/support/opengl40_errors.h>

namespace enishi::renderer::opengl {
    namespace {
        constexpr std::uint32_t NO_GL_OBJECT = 0;
        constexpr std::uint32_t RESOURCE_COUNT = 1;
    } // namespace
    platform::RenderResult<types::RenderHandle> OpenGL40Buffers::make_buffer(
        OpenGL40RendererState& runtime,
        RenderHandleMapper& handle_mapper,
        const types::RenderData& data,
        const std::uint32_t target) {
        auto pending = check_gl_errors("before buffer creation");
        if (pending.is_err()) {
            return std::move(pending).take_err();
        }
        if (data.byte_width() >
            static_cast<std::size_t>((std::numeric_limits<GLsizeiptr>::max)())) {
            return foundation::Error(
                platform::RenderError::MakeError, "OpenGL buffer size exceeds GLsizeiptr range");
        }
        GLuint object = NO_GL_OBJECT;
        glGenBuffers(RESOURCE_COUNT, &object);
        // Element-array binding requires a VAO in a core context; upload before VAO creation.
        const auto upload_target =
            target == GL_ELEMENT_ARRAY_BUFFER ? GL_COPY_WRITE_BUFFER : target;
        glBindBuffer(upload_target, object);
        glBufferData(upload_target,
            static_cast<GLsizeiptr>(data.byte_width()),
            data.raw_data(),
            GL_DYNAMIC_DRAW);
        auto result = check_gl_errors("buffer allocation");
        if (result.is_err()) {
            glDeleteBuffers(RESOURCE_COUNT, &object);
            return std::move(result).take_err();
        }
        runtime.buffers.emplace_back(object);
        const auto handle =
            OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::Buffer);
        runtime.objects.emplace(handle, object);
        return handle;
    }
} // namespace enishi::renderer::opengl
