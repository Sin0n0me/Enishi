#include "vulkan_render_command_encoder.h"
#include <renderer/vulkan/command/vulkan_commands.h>

namespace enishi::renderer::vulkan {

    VulkanRenderCommandEncoder::VulkanRenderCommandEncoder(
        VulkanRendererState& state, ResourceAccessor& resource_accessor)
        : state(state)
        , resource_accessor(resource_accessor) {
    }
    void VulkanRenderCommandEncoder::setup_viewports() const {
        return VulkanCommands::setup_viewports();
    }
    void VulkanRenderCommandEncoder::setup_views() const {
        return VulkanCommands::setup_views(this->state, this->resource_accessor);
    }
    void VulkanRenderCommandEncoder::submit_command_shader(
        const types::DrawCommand& command) const {
        return VulkanCommands::submit_command_shader(this->state, command);
    }
    void VulkanRenderCommandEncoder::submit_command_state(const types::DrawCommand& command) const {
        return VulkanCommands::submit_command_state(this->state, command);
    }
    void VulkanRenderCommandEncoder::submit_command_view(
        const types::DrawCommand& command, const types::RenderHandle& render_target) const {
        return VulkanCommands::submit_command_view(
            this->state, this->resource_accessor, command, render_target);
    }
    void VulkanRenderCommandEncoder::submit_command_viewport(
        const types::DrawCommand& command) const {
        return VulkanCommands::submit_command_viewport(this->state, command);
    }
    void VulkanRenderCommandEncoder::submit_command_topology(
        const types::DrawCommand& command) const {
        return VulkanCommands::submit_command_topology(this->state, command);
    }
    void VulkanRenderCommandEncoder::submit_command_vertex_layout(
        const types::DrawCommand& command) const {
        return VulkanCommands::submit_command_vertex_layout(command);
    }
    void VulkanRenderCommandEncoder::submit_command_buffer(
        const types::DrawCommand& command) const {
        return VulkanCommands::submit_command_buffer(this->state, command);
    }
    void VulkanRenderCommandEncoder::submit_command_image(const types::DrawCommand& command) const {
        return VulkanCommands::submit_command_image(this->state, command);
    }
    void VulkanRenderCommandEncoder::draw(const types::RenderHandle& handle) const {
        return VulkanCommands::draw(this->state, handle);
    }
    void VulkanRenderCommandEncoder::submit_command_mesh(
        const types::DrawCommand& command, const types::RenderHandle& render_target) const {
        return VulkanCommands::submit_command_mesh(this->state, command, render_target);
    }
    void VulkanRenderCommandEncoder::present() const {
        return VulkanCommands::present(this->state);
    }
} // namespace enishi::renderer::vulkan
