#pragma once
#include <renderer/common/render_handle_mapper.h>
#include <renderer/common/resource_accessor.h>
#include <renderer/opengl/opengl40/runtime/opengl40_renderer_state.h>

namespace enishi::renderer::opengl {
    class OpenGL40RenderStates final {
      public:
        static platform::RenderResult<types::RenderHandle> create_rasterizer(
            OpenGL40RendererState& runtime,
            ResourceAccessor& resource_accessor,
            RenderHandleMapper& handle_mapper,
            const types::RasterizerStateDescription& state);
        static platform::RenderResult<types::RenderHandle> create_depth_stencil(
            OpenGL40RendererState& runtime,
            ResourceAccessor& resource_accessor,
            RenderHandleMapper& handle_mapper,
            const types::DepthStencilStateDescription& state);
        static platform::RenderResult<types::RenderHandle> create_sampler(
            OpenGL40RendererState& runtime,
            ResourceAccessor& resource_accessor,
            RenderHandleMapper& handle_mapper,
            const types::SamplerStateDescription& state);
        static platform::RenderResult<types::RenderHandle> create_blend(
            OpenGL40RendererState& runtime,
            ResourceAccessor& resource_accessor,
            RenderHandleMapper& handle_mapper,
            const types::BlendStateDescription& state);
        static void submit_command_state(
            OpenGL40RendererState& runtime, const types::DrawCommand& command);

      private:
        static void apply_rasterizer(const types::RasterizerStateDescription& state);
        static void apply_depth_stencil(const types::DepthStencilStateDescription& state);
        static void apply_blend(const types::BlendStateDescription& state);
    };
} // namespace enishi::renderer::opengl
