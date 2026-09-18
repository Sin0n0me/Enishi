#include "vulkan_renderer.h"

namespace enishi::renderer::vulkan {
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_mesh(
        const types::ModelData& model_data, const std::vector<types::RenderHandle>& shaders) {
        return types::RenderHandle{};
    }

    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_texture(
        const types::TextureData& texture) {
        return types::RenderHandle{};
    }

    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_shader(
        const types::ShaderKind kind, const types::ShaderData& shader) {
        return types::RenderHandle{};
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_viewport(
        const types::ViewportRect& config) {
        return types::RenderHandle{};
    }
    platform::RenderResult<std::unique_ptr<platform::IPipelineLayout>>
    VulkanRenderer::create_vertex_layout(const types::VertexLayout& layout,
        const types::RenderHandle& vertex_shader,
        const types::RenderHandle& pixel_shader) {
        return std::unique_ptr<platform::IPipelineLayout>{};
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_shader_reflection(
        const types::ShaderData& shader_data) {
        return types::RenderHandle{};
    }
    platform::RenderResult<types::RenderHandle>
    VulkanRenderer::create_vertex_layout_from_shader_data(const types::ShaderData& shader) {
        return types::RenderHandle{};
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_rasterizer(
        const types::RasterizerStateDescription& description) {
        return types::RenderHandle{};
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_image(
        const types::ImageDescription& description) {
        return types::RenderHandle{};
    }
    platform::RenderResult<std::shared_ptr<platform::IRenderTargetView>>
    VulkanRenderer::create_render_target_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        return std::shared_ptr<platform::IRenderTargetView>{};
    }
    platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>>
    VulkanRenderer::create_depth_stencil_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        return std::shared_ptr<platform::IDepthStencilView>{};
    }
    platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>>
    VulkanRenderer::create_shader_resource_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        return std::shared_ptr<platform::IShaderResourceView>{};
    }
    platform::RenderResult<std::shared_ptr<platform::IUnorderedAccessView>>
    VulkanRenderer::create_unordered_access_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        return std::shared_ptr<platform::IUnorderedAccessView>{};
    }
} // namespace enishi::renderer::vulkan