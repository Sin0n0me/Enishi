#include "opengl_uniform_updater.h"
#include <glad/gl.h>

namespace enishi::renderer::opengl {
    constexpr GLintptr UNIFORM_BUFFER_DATA_OFFSET = 0;

    OpenGLUniformUpdater::OpenGLUniformUpdater(types::OwnedRenderData&& resource,
        const std::uint32_t buffer,
        const std::uint32_t binding) noexcept
        : resource(std::move(resource))
        , buffer(buffer)
        , binding(binding) {
    }
    void OpenGLUniformUpdater::on_update(void) {
        const auto data = this->resource.get_render_data();
        glBindBuffer(GL_UNIFORM_BUFFER, this->buffer);
        glBufferSubData(GL_UNIFORM_BUFFER,
            UNIFORM_BUFFER_DATA_OFFSET,
            static_cast<GLsizeiptr>(data.byte_width()),
            data.raw_data());
        glBindBufferBase(GL_UNIFORM_BUFFER, this->binding, this->buffer);
    }
    types::OwnedRenderData& OpenGLUniformUpdater::get_resource(void) {
        return this->resource;
    }
} // namespace enishi::renderer::opengl
