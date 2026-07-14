#include "vulkan_renderer.h"

namespace enishi::renderer::vulkan {
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_mesh(
        const types::MeshData& mesh) {
        return {};
    }

    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_texture(
        const types::TextureData& texture) {
        return {};
    }

    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_shader(
        const types::ShaderKind kind, const types::ShaderData& shader) {
        return {};
    }

    void VulkanRenderer::submit_render_graph(const types::RenderGraph& graph) {
    }

    void VulkanRenderer::present(void) {
    }
} // namespace enishi::renderer::vulkan