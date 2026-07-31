#include "vulkan_renderer.h"

namespace enishi::renderer::vulkan {
    platform::RenderResult<types::RenderHandle> VulkanRenderer::create_mesh(
        const types::MeshData& mesh) {
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

    void VulkanRenderer::submit_render_graph(const types::RenderGraph& graph) {
    }

    void VulkanRenderer::present(void) {
    }
} // namespace enishi::renderer::vulkan