#include "vulkan_render_command_encoder.h"
#include <renderer/vulkan/command/vulkan_commands.h>
#include <utility>

namespace enishi::renderer::vulkan {

    VulkanRenderCommandEncoder::VulkanRenderCommandEncoder(
        std::shared_ptr<IVulkanRenderResources> resources)
        : resources(std::move(resources)) {
    }
    void VulkanRenderCommandEncoder::setup_viewports() const {
        return VulkanCommands::setup_viewports();
    }
    void VulkanRenderCommandEncoder::setup_views() const {
        return VulkanCommands::setup_views(
            this->resources->get_state(), this->resources->get_resource_accessor());
    }
    void VulkanRenderCommandEncoder::submit_command_shader(
        const types::DrawCommand& command) const {
        return VulkanCommands::submit_command_shader(this->resources->get_state(), command);
    }
    void VulkanRenderCommandEncoder::submit_command_state(const types::DrawCommand& command) const {
        return VulkanCommands::submit_command_state(this->resources->get_state(), command);
    }
    void VulkanRenderCommandEncoder::submit_command_view(
        const types::DrawCommand& command, const types::RenderHandle& render_target) const {
        return VulkanCommands::submit_command_view(this->resources->get_state(),
            this->resources->get_resource_accessor(),
            command,
            render_target);
    }
    void VulkanRenderCommandEncoder::submit_command_viewport(
        const types::DrawCommand& command) const {
        return VulkanCommands::submit_command_viewport(this->resources->get_state(), command);
    }
    void VulkanRenderCommandEncoder::submit_command_topology(
        const types::DrawCommand& command) const {
        return VulkanCommands::submit_command_topology(this->resources->get_state(), command);
    }
    void VulkanRenderCommandEncoder::submit_command_vertex_layout(
        const types::DrawCommand& command) const {
        return VulkanCommands::submit_command_vertex_layout(command);
    }
    void VulkanRenderCommandEncoder::submit_command_buffer(
        const types::DrawCommand& command) const {
        return VulkanCommands::submit_command_buffer(this->resources->get_state(), command);
    }
    void VulkanRenderCommandEncoder::submit_command_image(const types::DrawCommand& command) const {
        return VulkanCommands::submit_command_image(this->resources->get_state(), command);
    }
    void VulkanRenderCommandEncoder::draw(const types::RenderHandle& handle) const {
        return VulkanCommands::draw(this->resources->get_state(), handle);
    }
    void VulkanRenderCommandEncoder::submit_command_mesh(
        const types::DrawCommand& command, const types::RenderHandle& render_target) const {
        return VulkanCommands::submit_command_mesh(
            this->resources->get_state(), command, render_target);
    }
    void VulkanRenderCommandEncoder::present() const {
        return VulkanCommands::present(this->resources->get_state());
    }
} // namespace enishi::renderer::vulkan
