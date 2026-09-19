#pragma once

#include "opengl40_context.h"
#include <renderer/opengl/common/opengl_resource_accessor.h>
#include <renderer/common/render_handle_mapper.h>
#include <platform/renderer/interface_render_command_encoder.h>
#include <platform/renderer/interface_renderer.h>
#include <unordered_map>

namespace enishi::renderer::opengl {
    class OpenGL40Renderer final : public platform::IRenderer, public platform::IRenderCommandEncoder {
      private:
        std::shared_ptr<OpenGL40Context> context;
        std::unique_ptr<RenderHandleMapper> handle_mapper;
        std::unique_ptr<OpenGLResourceAccessor> resource_accessor;
        mutable std::unordered_map<types::RenderHandle, std::uint32_t> objects;
        mutable std::unordered_map<types::RenderHandle, types::ShaderKind> shader_kinds;
        mutable std::unordered_map<types::RenderHandle, types::DrawBinding> draw_bindings;
        mutable std::unordered_map<types::RenderHandle, std::uint32_t> index_types;
        mutable std::unordered_map<types::RenderHandle, types::ImageDescription> images;
        mutable std::unordered_map<types::RenderHandle, types::RasterizerStateDescription> rasterizers;
        mutable std::unordered_map<types::RenderHandle, types::DepthStencilStateDescription> depth_stencils;
        mutable std::unordered_map<types::RenderHandle, types::BlendStateDescription> blends;
        mutable std::unordered_map<types::RenderHandle, types::SamplerStateDescription> samplers;
        mutable std::uint32_t topology = 0;
        mutable std::uint32_t active_vertex_shader = 0;
        mutable std::uint32_t active_fragment_shader = 0;
        mutable std::uint32_t active_program = 0;
        mutable std::uint32_t active_framebuffer = 0;

      public:
        explicit OpenGL40Renderer(std::shared_ptr<OpenGL40Context> context);

        platform::RenderResult<types::RenderHandle> create_viewport(const types::ViewportRect&) override;
        platform::RenderResult<types::RenderHandle> create_shader_reflection(const types::ShaderData&) override;
        platform::RenderResult<std::unique_ptr<platform::IPipelineLayout>> create_vertex_layout(
            const types::VertexLayout&, const types::RenderHandle&, const types::RenderHandle&) override;
        platform::RenderResult<types::RenderHandle> create_vertex_layout_from_shader_data(const types::ShaderData&) override;
        platform::RenderResult<types::RenderHandle> create_rasterizer(const types::RasterizerStateDescription&) override;
        platform::RenderResult<types::RenderHandle> create_depth_stencil(const types::DepthStencilStateDescription&) override;
        platform::RenderResult<types::RenderHandle> create_sampler(const types::SamplerStateDescription&) override;
        platform::RenderResult<types::RenderHandle> create_blend(const types::BlendStateDescription&) override;
        platform::RenderResult<types::RenderHandle> create_image(const types::ImageDescription&) override;
        platform::RenderResult<std::shared_ptr<platform::IRenderTargetView>> create_render_target_view(types::RenderHandle, const types::ImageViewDescription&) override;
        platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>> create_depth_stencil_view(types::RenderHandle, const types::ImageViewDescription&) override;
        platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>> create_shader_resource_view(types::RenderHandle, const types::ImageViewDescription&) override;
        platform::RenderResult<std::shared_ptr<platform::IUnorderedAccessView>> create_unordered_access_view(types::RenderHandle, const types::ImageViewDescription&) override;
        platform::RenderResult<types::RenderHandle> create_mesh(const types::ModelData&, const std::vector<types::RenderHandle>&) override;
        platform::RenderResult<types::RenderHandle> create_texture(const types::TextureData&) override;
        platform::RenderResult<types::RenderHandle> create_shader(types::ShaderKind, const types::ShaderData&) override;
        platform::IRenderResourceAccessor* get_resource_accessor(void) noexcept override;
        platform::IRenderResourceAccessor* const get_resource_accessor(void) const noexcept override;
        const platform::IRenderHandleMapper* get_handle_mapper(void) const noexcept override;

        void setup_viewports(void) const override;
        void setup_views(void) const override;
        void submit_command_buffer(const types::DrawCommand&) const override;
        void submit_command_shader(const types::DrawCommand&) const override;
        void submit_command_view(const types::DrawCommand&, const types::RenderHandle&) const override;
        void submit_command_viewport(const types::DrawCommand&) const override;
        void submit_command_mesh(const types::DrawCommand&, const types::RenderHandle&) const override;
        void submit_command_topology(const types::DrawCommand&) const override;
        void submit_command_vertex_layout(const types::DrawCommand&) const override;
        void submit_command_state(const types::DrawCommand&) const override;
        void submit_command_image(const types::DrawCommand&) const override;
        void draw(const types::RenderHandle&) const override;
        void present(void) const override;

      private:
        [[nodiscard]] types::RenderHandle make_handle(types::RenderHandleType type) noexcept;
        [[nodiscard]] platform::RenderResult<types::RenderHandle> make_buffer(
            const types::RenderData& data, std::uint32_t target);
        [[nodiscard]] bool use_active_program(void) const;
    };
} // namespace enishi::renderer::opengl
