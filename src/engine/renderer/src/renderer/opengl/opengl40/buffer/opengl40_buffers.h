#pragma once
#include <renderer/common/render_handle_mapper.h>
#include <renderer/opengl/opengl40/runtime/opengl40_renderer_state.h>

namespace enishi::renderer::opengl {
    class OpenGL40Buffers final {
      public:
        static platform::RenderResult<types::RenderHandle> make_buffer(
            OpenGL40RendererState& runtime,
            RenderHandleMapper& handle_mapper,
            const types::RenderData& data,
            const GLenum target);
    };
} // namespace enishi::renderer::opengl
