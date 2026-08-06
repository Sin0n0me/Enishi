#pragma once
#include "d3d11.h"
#include "resource/resource_manager.h"
#include "resource/shader.h"
#include <engine_types/handle/handle_allocator.h>
#include <engine_types/renderer/render_handle.h>
#include <memory>
#include <platform/renderer/interface_render_command_encoder.h>
#include <platform/renderer/interface_renderer.h>

namespace enishi::renderer::directx {
    class D3D11Renderer : public platform::IRenderer, public platform::IRenderCommandEncoder {
      private:
        std::shared_ptr<D3D11> d3d11;
        std::unique_ptr<ResourceManager> resource_manager;

      public:
        explicit D3D11Renderer(std::unique_ptr<D3D11> d3d11);

      public:
        platform::RenderResult<types::RenderPass> create_render_pass(
            const types::PipelineDescription& description) override;
        platform::RenderResult<types::RenderHandle> create_viewport(
            const types::ViewportRect& config) override;
        platform::RenderResult<std::unique_ptr<platform::IPipelineLayout>> create_pipeline_layout(
            const types::VertexLayout& layout,
            const types::RenderHandle& vertex_shader,
            const types::RenderHandle& pixel_shader) override;
        platform::RenderResult<types::RenderHandle> create_pipeline_layout_from_shader(
            const types::ShaderData& shader) override;
        platform::RenderResult<types::RenderHandle> create_rasterizer(
            const types::RasterizerDescription& description) override;
        platform::RenderResult<types::RenderHandle> create_image(
            const types::ImageDescription& description) override;
        platform::RenderResult<std::weak_ptr<platform::IRenderTargetView>>
        create_render_target_view(types::RenderHandle image_handle,
            const types::ImageViewDescription& description) override;
        platform::RenderResult<std::weak_ptr<platform::IDepthStencilView>>
        create_depth_stencil_view(types::RenderHandle image_handle,
            const types::ImageViewDescription& description) override;
        platform::RenderResult<std::weak_ptr<platform::IShaderResourceView>>
        create_shader_resource_view(types::RenderHandle image_handle,
            const types::ImageViewDescription& description) override;
        platform::RenderResult<std::weak_ptr<platform::IUnorderedAccessView>>
        create_unordered_access_view(types::RenderHandle image_handle,
            const types::ImageViewDescription& description) override;
        platform::RenderResult<types::RenderHandle> create_mesh(types::MeshData&& mesh) override;
        platform::RenderResult<types::RenderHandle> create_texture(
            const types::TextureData& texture) override;
        platform::RenderResult<types::RenderHandle> create_shader(
            const types::ShaderKind kind, const types::ShaderData& shader_data) override;

      public:
        void setup_viewports(void) const override;
        void setup_render_targets(void) const override;
        void bind_buffer(const types::HandleId id) const override;
        void bind_shader(const types::HandleId id) const override;
        void bind_view(const types::HandleId id) const override;
        void bind_rasterizer(const types::HandleId id) const override;
        void bind_texture(const types::HandleId id) const override;
        void bind_mesh(const types::HandleId id) const override;
        void bind_topology(const types::HandleId id) const override;
        void bind_input_layout(const types::HandleId id) const override;
        void draw(const types::HandleId id) const override;
        void present(void) const override;
    };
} // namespace enishi::renderer::directx
