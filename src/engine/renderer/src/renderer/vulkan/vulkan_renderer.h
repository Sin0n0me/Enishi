#pragma once
#include <engine_types/handle/handle_allocator.h>
#include <engine_types/renderer/render_handle.h>
#include <memory>
#include <platform/renderer/interface_render_initializer.h>
#include <platform/renderer/interface_renderer.h>
#include <unordered_map>

namespace enishi::renderer::vulkan {
    class VulkanRenderer : public platform::IRenderer {
      private:
      public:
        platform::RenderResult<types::RenderHandle> create_mesh(
            const types::MeshData& mesh) override;
        platform::RenderResult<types::RenderHandle> create_texture(
            const types::TextureData& texture) override;
        platform::RenderResult<types::RenderHandle> create_shader(
            const types::ShaderKind kind, const types::ShaderData& shader) override;
        void submit_render_graph(const types::RenderGraph& graph) override;
        void present(void) override;
    };
} // namespace enishi::renderer::vulkan
