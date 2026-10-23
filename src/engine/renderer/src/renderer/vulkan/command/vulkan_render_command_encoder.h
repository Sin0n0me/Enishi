#pragma once
#include <platform/renderer/interface_render_command_encoder.h>
#include <renderer/common/resource_accessor.h>
#include <renderer/vulkan/runtime/vulkan_renderer_state.h>

namespace enishi::renderer::vulkan {
    class VulkanRenderCommandEncoder final : public platform::IRenderCommandEncoder {
      private:
        VulkanRendererState& state;
        ResourceAccessor& resource_accessor;

      public:
        VulkanRenderCommandEncoder(VulkanRendererState& state, ResourceAccessor& resource_accessor);
        void setup_viewports(void) const override;
        void setup_views(void) const override;
        void submit_command_buffer(const types::DrawCommand&) const override;
        void submit_command_shader(const types::DrawCommand&) const override;
        void submit_command_view(
            const types::DrawCommand&, const types::RenderHandle&) const override;
        void submit_command_viewport(const types::DrawCommand&) const override;
        void submit_command_mesh(
            const types::DrawCommand&, const types::RenderHandle&) const override;
        void submit_command_topology(const types::DrawCommand&) const override;
        void submit_command_vertex_layout(const types::DrawCommand&) const override;
        void submit_command_state(const types::DrawCommand&) const override;
        void submit_command_image(const types::DrawCommand&) const override;
        void draw(const types::RenderHandle&) const override;
        void present(void) const override;
    };
} // namespace enishi::renderer::vulkan
