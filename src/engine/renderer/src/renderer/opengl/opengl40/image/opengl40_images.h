#pragma once
#include <renderer/common/render_handle_mapper.h>
#include <renderer/opengl/opengl40/runtime/opengl40_renderer_state.h>

namespace enishi::renderer::opengl {
    class OpenGL40Images final {
      public:
        static platform::RenderResult<types::RenderHandle> create_image(
            OpenGL40RendererState& runtime,
            RenderHandleMapper& handle_mapper,
            const types::ImageDescription& description);
        static platform::RenderResult<types::RenderHandle> create_texture(
            OpenGL40RendererState& runtime,
            RenderHandleMapper& handle_mapper,
            const types::TextureData& texture);
    };
} // namespace enishi::renderer::opengl
