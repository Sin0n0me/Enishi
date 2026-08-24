#pragma once
#include <engine_types/handle/handle_allocator.h>
#include <engine_types/handle/renderer/render_handle.h>
#include <memory>
#include <platform/renderer/interface_render_initializer.h>
#include <platform/renderer/interface_renderer.h>
#include <unordered_map>

namespace enishi::renderer::vulkan {
    class VulkanRenderer : public platform::IRenderer {
      private:
      public:
        platform::RenderResult<types::RenderHandle> create_mesh(const types::ModelData& model_data,
            const std::vector<types::RenderHandle>& shaders) override;
        platform::RenderResult<types::RenderHandle> create_texture(
            const types::TextureData& texture) override;
        platform::RenderResult<types::RenderHandle> create_shader(
            const types::ShaderKind kind, const types::ShaderData& shader) override;
        platform::RenderResult<types::RenderHandle> create_viewport(
            const types::ViewportRect& config) override;
        platform::RenderResult<types::RenderHandle> create_shader_reflection(
            const types::ShaderData& shader_data) override;
        platform::RenderResult<types::RenderHandle> create_vertex_layout_from_shader_data(
            const types::ShaderData& shader) override;
        platform::RenderResult<std::unique_ptr<platform::IPipelineLayout>> create_vertex_layout(
            const types::VertexLayout& layout,
            const types::RenderHandle& vertex_shader,
            const types::RenderHandle& pixel_shader) override;
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
    };
} // namespace enishi::renderer::vulkan
