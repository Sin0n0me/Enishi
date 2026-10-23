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
    class D3D11RenderCommandEncoder;

    class D3D11Renderer : public platform::IRenderer {
      private:
        std::shared_ptr<D3D11> d3d11;
        std::unique_ptr<ResourceManager> resource_manager;
        std::unique_ptr<D3D11RenderCommandEncoder> encoder;

      public:
        explicit D3D11Renderer(std::unique_ptr<D3D11> d3d11);
        ~D3D11Renderer() noexcept override;

        // Borrowed encoder; its lifetime is bounded by this renderer.
        [[nodiscard]] platform::IRenderCommandEncoder& get_command_encoder() const noexcept;

        [[nodiscard]] std::shared_ptr<ID3D11Context> get_native_context(void) const;

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
            const types::RasterizerStateDescription& description) override;
        platform::RenderResult<types::RenderHandle> create_depth_stencil(
            const types::DepthStencilStateDescription& description) override;
        platform::RenderResult<types::RenderHandle> create_sampler(
            const types::SamplerStateDescription& description) override;
        platform::RenderResult<types::RenderHandle> create_blend(
            const types::BlendStateDescription& description) override;
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
        platform::RenderResult<types::RenderHandle> create_texture(
            const types::TextureData& texture) override;
        platform::RenderResult<types::RenderHandle> create_shader(
            const types::ShaderKind kind, const types::ShaderData& shader_data) override;
        platform::IRenderResourceAccessor* get_resource_accessor(void) noexcept override;
        platform::IRenderResourceAccessor* const get_resource_accessor(
            void) const noexcept override;
        const platform::IRenderHandleMapper* get_handle_mapper(void) const noexcept override;
    };
} // namespace enishi::renderer::directx
