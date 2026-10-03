#include "vulkan_renderer.h"

namespace enishi::renderer::vulkan {
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_mesh(
        const types::ModelData& model_data, const std::vector<types::RenderHandle>& shaders) {
        return foundation::Error(
            platform::RenderError::MakeError, "Vulkan create_mesh is not implemented");
    }

    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_texture(
        const types::TextureData& texture) {
        return foundation::Error(
            platform::RenderError::MakeError, "Vulkan create_texture is not implemented");
    }

    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_shader(
        const types::ShaderKind kind, const types::ShaderData& shader) {
        return foundation::Error(
            platform::RenderError::MakeError, "Vulkan create_shader is not implemented");
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_viewport(
        const types::ViewportRect& config) {
        return foundation::Error(
            platform::RenderError::MakeError, "Vulkan create_viewport is not implemented");
    }
    platform::RenderResult<std::unique_ptr<platform::IPipelineLayout>>
    VulkanRenderer::create_vertex_layout(const types::VertexLayout& layout,
        const types::RenderHandle& vertex_shader,
        const types::RenderHandle& pixel_shader) {
        return foundation::Error(
            platform::RenderError::MakeError, "Vulkan create_vertex_layout is not implemented");
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_shader_reflection(
        const types::ShaderData& shader_data) {
        return foundation::Error(
            platform::RenderError::MakeError, "Vulkan create_shader_reflection is not implemented");
    }
    platform::RenderResult<types::RenderHandle>
    VulkanRenderer::create_vertex_layout_from_shader_data(const types::ShaderData& shader) {
        return foundation::Error(platform::RenderError::MakeError,
            "Vulkan create_vertex_layout_from_shader_data is not implemented");
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_rasterizer(
        const types::RasterizerStateDescription& description) {
        return foundation::Error(
            platform::RenderError::MakeError, "Vulkan create_rasterizer is not implemented");
    }
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_image(
        const types::ImageDescription& description) {
        return foundation::Error(
            platform::RenderError::MakeError, "Vulkan create_image is not implemented");
    }
    platform::RenderResult<std::shared_ptr<platform::IRenderTargetView>>
    VulkanRenderer::create_render_target_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        return foundation::Error(platform::RenderError::MakeError,
            "Vulkan create_render_target_view is not implemented");
    }
    platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>>
    VulkanRenderer::create_depth_stencil_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        return foundation::Error(platform::RenderError::MakeError,
            "Vulkan create_depth_stencil_view is not implemented");
    }
    platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>>
    VulkanRenderer::create_shader_resource_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        return foundation::Error(platform::RenderError::MakeError,
            "Vulkan create_shader_resource_view is not implemented");
    }
    platform::RenderResult<std::shared_ptr<platform::IUnorderedAccessView>>
    VulkanRenderer::create_unordered_access_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        return foundation::Error(platform::RenderError::MakeError,
            "Vulkan create_unordered_access_view is not implemented");
    }
} // namespace enishi::renderer::vulkan