#pragma once

#include <memory>
#include <platform/renderer/interface_opengl_context.h>
#include <platform/renderer/interface_render_command_encoder.h>
#include <platform/renderer/interface_renderer.h>
#include <renderer/common/render_handle_mapper.h>
#include <renderer/common/resource_accessor.h>
#include <renderer/opengl/opengl40/runtime/opengl40_renderer_state.h>
#include <vector>

namespace enishi::renderer::opengl {
    class OpenGL40RenderCommandEncoder;

    class OpenGL40Renderer final : public platform::IRenderer {
      private:
        std::shared_ptr<platform::IOpenGLContext> context;
        std::unique_ptr<RenderHandleMapper> handle_mapper;
        std::unique_ptr<ResourceAccessor> resource_accessor;
        std::unique_ptr<OpenGL40RendererState> state;
        std::unique_ptr<OpenGL40RenderCommandEncoder> encoder;

      public:
        explicit OpenGL40Renderer(std::shared_ptr<platform::IOpenGLContext> context);
        ~OpenGL40Renderer(void) noexcept override;

        // Borrowed encoder; its lifetime is bounded by this renderer.
        [[nodiscard]] platform::IRenderCommandEncoder& get_command_encoder() const noexcept;

        platform::RenderResult<types::RenderHandle> create_viewport(
            const types::ViewportRect&) override;
        platform::RenderResult<types::RenderHandle> create_shader_reflection(
            const types::ShaderData&) override;
        platform::RenderResult<std::unique_ptr<platform::IPipelineLayout>> create_vertex_layout(
            const types::VertexLayout&,
            const types::RenderHandle&,
            const types::RenderHandle&) override;
        platform::RenderResult<types::RenderHandle> create_vertex_layout_from_shader_data(
            const types::ShaderData&) override;
        platform::RenderResult<types::RenderHandle> create_rasterizer(
            const types::RasterizerStateDescription&) override;
        platform::RenderResult<types::RenderHandle> create_depth_stencil(
            const types::DepthStencilStateDescription&) override;
        platform::RenderResult<types::RenderHandle> create_sampler(
            const types::SamplerStateDescription&) override;
        platform::RenderResult<types::RenderHandle> create_blend(
            const types::BlendStateDescription&) override;
        platform::RenderResult<types::RenderHandle> create_image(
            const types::ImageDescription&) override;
        platform::RenderResult<std::shared_ptr<platform::IRenderTargetView>>
        create_render_target_view(types::RenderHandle, const types::ImageViewDescription&) override;
        platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>>
        create_depth_stencil_view(types::RenderHandle, const types::ImageViewDescription&) override;
        platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>>
        create_shader_resource_view(
            types::RenderHandle, const types::ImageViewDescription&) override;
        platform::RenderResult<std::shared_ptr<platform::IUnorderedAccessView>>
        create_unordered_access_view(
            types::RenderHandle, const types::ImageViewDescription&) override;
        platform::RenderResult<types::RenderHandle> create_mesh(
            const types::ModelData&, const std::vector<types::RenderHandle>&) override;
        platform::RenderResult<types::RenderHandle> create_texture(
            const types::TextureData&) override;
        platform::RenderResult<types::RenderHandle> create_shader(
            types::ShaderKind, const types::ShaderData&) override;
        platform::IRenderResourceAccessor* get_resource_accessor(void) noexcept override;
        platform::IRenderResourceAccessor* const get_resource_accessor(
            void) const noexcept override;
        const platform::IRenderHandleMapper* get_handle_mapper(void) const noexcept override;
    };
} // namespace enishi::renderer::opengl
