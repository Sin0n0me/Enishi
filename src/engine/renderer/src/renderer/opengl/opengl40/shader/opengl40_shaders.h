#pragma once
#include <renderer/common/render_handle_mapper.h>
#include <renderer/common/resource_accessor.h>
#include <renderer/opengl/opengl40/runtime/opengl40_renderer_state.h>

namespace enishi::renderer::opengl {
    class OpenGL40Shaders final {
      public:
        static platform::RenderResult<types::RenderHandle> create_shader_reflection(
            OpenGL40RendererState& runtime,
            ResourceAccessor& resource_accessor,
            RenderHandleMapper& handle_mapper,
            const types::ShaderData& data);
        static platform::RenderResult<types::RenderHandle> create_vertex_layout_from_shader_data(
            OpenGL40RendererState& runtime,
            ResourceAccessor& resource_accessor,
            RenderHandleMapper& handle_mapper,
            const types::ShaderData& data);
        static platform::RenderResult<types::RenderHandle> create_shader(
            OpenGL40RendererState& runtime,
            RenderHandleMapper& handle_mapper,
            const types::ShaderKind kind,
            const types::ShaderData& data);
        static bool use_active_program(OpenGL40RendererState& runtime);
        static void submit_command_shader(
            OpenGL40RendererState& runtime, const types::DrawCommand& command);
    };
} // namespace enishi::renderer::opengl
