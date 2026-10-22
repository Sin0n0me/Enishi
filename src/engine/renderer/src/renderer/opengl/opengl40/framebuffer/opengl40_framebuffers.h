#pragma once
#include <renderer/common/resource_accessor.h>
#include <renderer/opengl/opengl40/runtime/opengl40_renderer_state.h>

namespace enishi::renderer::opengl {
    class OpenGL40Framebuffers final {
      public:
        static void setup_views(OpenGL40RendererState& runtime);
        static void submit_command_view(OpenGL40RendererState& runtime,
            ResourceAccessor& resource_accessor,
            const types::DrawCommand& command,
            const types::RenderHandle& render_target);
        static void present(OpenGL40RendererState& runtime);

      private:
        static void bind_depth_only_view(OpenGL40RendererState& runtime,
            ResourceAccessor& resource_accessor,
            const types::RenderHandle& handle);
        static bool prepare_back_buffer_framebuffer(
            OpenGL40RendererState& runtime, const types::RenderHandle& render_target);
    };
} // namespace enishi::renderer::opengl
