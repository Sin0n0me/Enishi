#pragma once
#include <renderer/common/resource_accessor.h>
#include <renderer/opengl/opengl40/runtime/opengl40_renderer_state.h>

namespace enishi::renderer::opengl {
    class OpenGL40Commands final {
      public:
        static void submit_command_mesh(OpenGL40RendererState& runtime,
            ResourceAccessor& resource_accessor,
            const types::DrawCommand& command,
            const types::RenderHandle&);
        static void submit_command_topology(
            OpenGL40RendererState& runtime, const types::DrawCommand& command);
        static void submit_command_image(
            OpenGL40RendererState& runtime, const types::DrawCommand& command);
        static void draw(OpenGL40RendererState& runtime, const types::RenderHandle& handle);
    };
} // namespace enishi::renderer::opengl
