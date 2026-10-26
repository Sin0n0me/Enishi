#pragma once
#include <renderer/common/render_handle_mapper.h>
#include <renderer/opengl/opengl40/runtime/opengl40_renderer_state.h>

namespace enishi::renderer::opengl {
    class OpenGL40Viewports final {
      public:
        static platform::RenderResult<types::RenderHandle> create_viewport(
            OpenGL40RendererState& runtime,
            RenderHandleMapper& handle_mapper,
            const types::ViewportRect& viewport);
        static void setup_viewports(OpenGL40RendererState& runtime);
        static void submit_command_viewport(
            OpenGL40RendererState& runtime, const types::DrawCommand& command);
    };
} // namespace enishi::renderer::opengl
