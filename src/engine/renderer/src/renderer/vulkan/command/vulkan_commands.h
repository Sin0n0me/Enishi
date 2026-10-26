#pragma once
#include <renderer/common/resource_accessor.h>
#include <renderer/vulkan/runtime/vulkan_renderer_state.h>
namespace enishi::renderer::vulkan {
    class VulkanCommands final {
      public:
        static void setup_viewports();
        static void setup_views(VulkanRendererState& state, ResourceAccessor& resource_accessor);
        static void submit_command_shader(
            VulkanRendererState& state, const types::DrawCommand& command);
        static void submit_command_state(
            VulkanRendererState& state, const types::DrawCommand& command);
        static void submit_command_view(VulkanRendererState& state,
            ResourceAccessor& resource_accessor,
            const types::DrawCommand& command,
            const types::RenderHandle& render_target);
        static void submit_command_viewport(
            VulkanRendererState& state, const types::DrawCommand& command);
        static void submit_command_topology(
            VulkanRendererState& state, const types::DrawCommand& command);
        static void submit_command_vertex_layout(const types::DrawCommand&);
        static void submit_command_buffer(VulkanRendererState& state, const types::DrawCommand&);
        static void submit_command_image(VulkanRendererState& state, const types::DrawCommand&);
        static void draw(VulkanRendererState& state, const types::RenderHandle&);
        static void submit_command_mesh(VulkanRendererState& state,
            const types::DrawCommand& command,
            const types::RenderHandle&);
        static void present(VulkanRendererState& state);

      private:
        static platform::RenderResult<void> prepare_mesh_sampler(
            VulkanRendererState& state, VulkanMesh& mesh);
    };
} // namespace enishi::renderer::vulkan
