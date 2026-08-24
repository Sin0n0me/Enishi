#pragma once
#include "d3d11.h"
#include "resource/resource_manager.h"
#include "resource/shader/shader_pool.h"
#include <engine_types/handle/handle_allocator.h>
#include <engine_types/handle/renderer/render_handle.h>
#include <memory>
#include <platform/renderer/interface_render_command_encoder.h>
#include <platform/renderer/interface_renderer.h>
#include <renderer/common/resource_binder.h>

namespace enishi::renderer::directx {
    class D3D11Renderer : public platform::IRenderer, public platform::IRenderCommandEncoder {
      private:
        std::shared_ptr<D3D11> d3d11;
        std::unique_ptr<ResourceManager> resource_manager;
        std::unique_ptr<UpdaterPool> updater_pool;

      public:
        explicit D3D11Renderer(std::unique_ptr<D3D11> d3d11);

      public:
        platform::RenderResult<types::RenderHandle> create_viewport(
            const types::ViewportRect& config) override;
        platform::RenderResult<types::RenderHandle> create_shader_reflection(
            const types::ShaderData& shader_data) override;
        platform::RenderResult<std::unique_ptr<platform::IPipelineLayout>> create_vertex_layout(
            const types::VertexLayout& layout,
            const types::RenderHandle& vertex_shader,
            const types::RenderHandle& pixel_shader) override;
        platform::RenderResult<types::RenderHandle> create_vertex_layout_from_shader_data(
            const types::ShaderData& shader_data) override;
        platform::RenderResult<types::RenderHandle> create_rasterizer(
            const types::RasterizerDescription& description) override;
        platform::RenderResult<types::RenderHandle> create_image(
            const types::ImageDescription& description) override;
        platform::RenderResult<std::shared_ptr<platform::IRenderTargetView>>
        create_render_target_view(types::RenderHandle image_handle,
            const types::ImageViewDescription& description) override;
        platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>>
        create_depth_stencil_view(types::RenderHandle image_handle,
            const types::ImageViewDescription& description) override;
        platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>>
        create_shader_resource_view(types::RenderHandle image_handle,
            const types::ImageViewDescription& description) override;
        platform::RenderResult<std::shared_ptr<platform::IUnorderedAccessView>>
        create_unordered_access_view(types::RenderHandle image_handle,
            const types::ImageViewDescription& description) override;
        platform::RenderResult<types::RenderHandle> create_mesh(const types::ModelData& model_data,
            const std::vector<types::RenderHandle>& shader_reflections) override;
        platform::RenderResult<types::RenderHandle> create_sampler(
            const types::SamplerDescription& description) override;
        platform::RenderResult<types::RenderHandle> create_texture(
            const types::TextureData& texture) override;
        platform::RenderResult<types::RenderHandle> create_shader(
            const types::ShaderKind kind, const types::ShaderData& shader_data) override;

      public:
        void setup_viewports(void) const override;
        void setup_views(void) const override;
        void submit_command_buffer(const types::DrawCommand& command) const override;
        void submit_command_shader(const types::DrawCommand& command) const override;
        void submit_command_view(
            const types::DrawCommand& command, const types::RenderHandle& handle) const override;
        void submit_command_rasterizer(const types::DrawCommand& command) const override;
        void submit_command_texture(const types::DrawCommand& command) const override;
        void submit_command_mesh(const types::DrawCommand& command) const override;
        void submit_command_topology(const types::DrawCommand& command) const override;
        void submit_command_vertex_layout(const types::DrawCommand& command) const override;
        void draw(const types::RenderHandle& handle) const override;
        void present(void) const override;

      private:
        void bind_buffer(const types::RenderHandle& handle) const;
        void bind_shader(const types::RenderHandle& handle) const;
        void bind_view(const types::RenderHandle& bind_handle,
            const types::RenderHandle& render_target_handle) const;
        void bind_rasterizer(const types::RenderHandle& handle) const;
        void bind_texture(const types::RenderHandle& handle) const;
        void bind_mesh(const types::RenderHandle& handle) const;
        void bind_topology(const types::RenderHandle& handle) const;
        void bind_input_layout(const types::RenderHandle& handle) const;
    };
} // namespace enishi::renderer::directx
