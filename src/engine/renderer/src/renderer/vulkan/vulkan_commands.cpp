#include "vulkan_renderer.h"
#include <algorithm>
#include <foundation/log/logger.h>

namespace enishi::renderer::vulkan {
    namespace {
        void fail(VulkanRendererState& state, std::string message) {
            if (!state.failed) {
                foundation::Logger::error(std::move(message));
            }
            state.failed = true;
        }
    } // namespace
    void VulkanRenderer::setup_viewports() const {
        // The current drawable extent is applied after acquiring the swapchain image.
    }
    void VulkanRenderer::setup_views() const {
        if (this->state->failed) {
            return;
        }
        glm::vec4 clear_color{};
        const auto targets = this->resource_accessor->get_render_target_views();
        if (!targets.empty()) {
            clear_color = targets.front()->get_clear_color();
        }
        this->state->sampled_shadow = this->state->fallback_shadow;
        this->state->active_shadow.reset();
        this->state->main_viewport = {};
        auto result = this->context->begin_frame(clear_color);
        if (result.is_err()) {
            fail(*this->state, result.unwrap_err().get_message());
        }
    }
    void VulkanRenderer::submit_command_shader(const types::DrawCommand& command) const {
        const auto found = this->state->shaders.find(command.handle);
        if (command.sub_command != types::SubCommand::Bind || found == this->state->shaders.end()) {
            return;
        }
        if (found->second.kind == types::ShaderKind::Vertex) {
            this->state->vertex_shader = command.handle;
        }
        if (found->second.kind == types::ShaderKind::Pixel) {
            this->state->pixel_shader = command.handle;
        }
    }
    void VulkanRenderer::submit_command_state(const types::DrawCommand& command) const {
        if (command.sub_command != types::SubCommand::Bind) {
            return;
        }
        if (this->state->rasterizers.contains(command.handle)) {
            this->state->rasterizer = command.handle;
        }
        if (this->state->depths.contains(command.handle)) {
            this->state->depth = command.handle;
        }
        if (this->state->blends.contains(command.handle)) {
            this->state->blend = command.handle;
        }
        if (this->state->samplers.contains(command.handle)) {
            this->state->sampler = this->state->samplers.at(command.handle);
        }
    }
    void VulkanRenderer::submit_command_view(
        const types::DrawCommand& command, const types::RenderHandle& render_target) const {
        if (!this->context->recording || this->state->failed ||
            command.sub_command != types::SubCommand::Bind) {
            return;
        }
        const auto found = this->state->shadow_targets.find(command.handle);
        if (found == this->state->shadow_targets.end()) {
            return; // Main attachments are opened lazily, after the shadow pass.
        }
        const auto depth = this->resource_accessor->get_depth_stencil_view(command.handle.id);
        if (depth.is_some() && !render_target.is_valid()) {
            auto result =
                this->context->begin_shadow_pass(*found->second, depth.unwrap()->clear_depth());
            if (result.is_err()) {
                fail(*this->state, result.unwrap_err().get_message());
                return;
            }
            this->state->active_shadow = found->second;
        } else if (this->resource_accessor->get_shader_resource_view(command.handle.id).is_some()) {
            if (this->state->active_shadow != found->second) {
                fail(*this->state, "Sampled shadow must have been rendered in this frame");
                return;
            }
            this->context->begin_main_pass();
            this->state->sampled_shadow = found->second;
        }
    }
    void VulkanRenderer::submit_command_viewport(const types::DrawCommand& command) const {
        const auto found = this->state->viewports.find(command.handle);
        if (!this->context->recording || found == this->state->viewports.end()) {
            return;
        }
        if (!this->context->shadow_recording) {
            this->state->main_viewport = command.handle;
        }
        const auto& value = found->second;
        VkViewport viewport{value.left_top_x,
            value.left_top_y,
            value.width,
            value.height,
            value.min_depth,
            value.max_depth};
        vkCmdSetViewport(this->context->command, 0, 1, &viewport);
    }
    void VulkanRenderer::submit_command_topology(const types::DrawCommand& command) const {
        if (command.handle.id.handle_id !=
            static_cast<std::uint32_t>(types::PrimitiveTopology::TriangleList)) {
            fail(*this->state, "Vulkan model pass requires triangle-list topology");
        }
    }
    void VulkanRenderer::submit_command_vertex_layout(const types::DrawCommand&) const {
        // Layout was validated against SkinnedVertex during render-pass construction.
    }
    void VulkanRenderer::submit_command_buffer(const types::DrawCommand&) const {
        fail(*this->state, "Vulkan buffer commands must be submitted through a model mesh");
    }
    void VulkanRenderer::submit_command_image(const types::DrawCommand&) const {
        fail(*this->state, "Vulkan textures must be submitted through material descriptors");
    }
    void VulkanRenderer::draw(const types::RenderHandle&) const {
        fail(*this->state, "Vulkan draws must be submitted through a model mesh");
    }
    void VulkanRenderer::submit_command_mesh(
        const types::DrawCommand& command, const types::RenderHandle&) const {
        if (!this->context->recording || this->state->failed ||
            command.sub_command != types::SubCommand::Bind) {
            return;
        }
        const auto found = this->state->meshes.find(command.handle);
        if (found == this->state->meshes.end()) {
            fail(*this->state, "Invalid Vulkan mesh handle");
            return;
        }
        if (!this->context->shadow_recording) {
            this->context->begin_main_pass();
            // Opening the main pass restores its extent; preserve an explicitly submitted viewport.
            if (this->state->main_viewport.is_valid()) {
                this->submit_command_viewport(
                    {.handle = this->state->main_viewport, .sub_command = types::SubCommand::Bind});
            }
        }
        auto pipeline = this->active_pipeline();
        if (pipeline.is_err()) {
            fail(*this->state, pipeline.unwrap_err().get_message());
            return;
        }
        auto& mesh = *found->second;
        auto prepared = this->prepare_mesh_sampler(mesh);
        if (prepared.is_err()) {
            fail(*this->state, prepared.unwrap_err().get_message());
            return;
        }
        const auto& descriptors = mesh.sampler_descriptors.at(this->state->sampler);
        const auto buffer = this->context->command;
        vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.unwrap());
        const auto& shadow = this->context->shadow_recording ? this->state->fallback_shadow
                                                             : this->state->sampled_shadow;
        constexpr std::uint32_t SHADOW_SET = 1;
        vkCmdBindDescriptorSets(buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            this->state->pipeline_layout,
            SHADOW_SET,
            1,
            &shadow->descriptors,
            0,
            nullptr);
        constexpr VkDeviceSize VERTEX_OFFSET = 0;
        vkCmdBindVertexBuffers(buffer, 0, 1, &mesh.vertices->buffer, &VERTEX_OFFSET);
        vkCmdBindIndexBuffer(buffer, mesh.indices->buffer, 0, mesh.index_type);
        for (std::size_t index = 0; index < mesh.materials.size(); ++index) {
            const auto& material = mesh.materials[index];
            vkCmdBindDescriptorSets(buffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                this->state->pipeline_layout,
                0,
                1,
                &descriptors[index],
                0,
                nullptr);
            constexpr std::uint32_t MINIMUM_INSTANCE_COUNT = 1;
            if (const auto indexed =
                    std::get_if<types::DrawIndexedParameter>(&material.draw.parameter);
                indexed != nullptr) {
                vkCmdDrawIndexed(buffer,
                    indexed->index_count,
                    (std::max)(MINIMUM_INSTANCE_COUNT, indexed->instance_count),
                    static_cast<std::uint32_t>(indexed->first_index),
                    static_cast<std::int32_t>(indexed->vertex_offset),
                    indexed->first_instance);
            } else if (const auto plain =
                           std::get_if<types::DrawParameter>(&material.draw.parameter);
                plain != nullptr) {
                vkCmdDraw(buffer,
                    plain->vertex_count,
                    (std::max)(MINIMUM_INSTANCE_COUNT, plain->instance_count),
                    plain->first_vertex,
                    plain->first_instance);
            }
        }
    }
    platform::RenderResult<void> VulkanRenderer::prepare_mesh_sampler(VulkanMesh& mesh) const {
        if (mesh.sampler_descriptors.contains(this->state->sampler)) {
            return {};
        }
        // Keep sets immutable: updating a set already recorded invalidates that command buffer.
        constexpr std::uint32_t UNIFORM_COUNT = 3;
        constexpr std::uint32_t TEXTURE_COUNT = 3;
        constexpr std::uint32_t DESCRIPTOR_COUNT = UNIFORM_COUNT + TEXTURE_COUNT + 1;
        const auto count = static_cast<std::uint32_t>(mesh.materials.size());
        // One set per material, including the shadow projection uniform.
        const std::array<VkDescriptorPoolSize, 2> sizes{
            {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count * (UNIFORM_COUNT + 1)},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count * TEXTURE_COUNT}}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool.maxSets = count;
        pool.poolSizeCount = static_cast<std::uint32_t>(sizes.size());
        pool.pPoolSizes = sizes.data();
        mesh.sampler_pools.emplace_back(VK_NULL_HANDLE);
        auto result =
            check_vk(vkCreateDescriptorPool(
                         this->context->device, &pool, nullptr, &mesh.sampler_pools.back()),
                "create sampler descriptor pool");
        if (result.is_err()) {
            return result;
        }
        std::vector<VkDescriptorSetLayout> layouts(count, this->state->descriptors);
        std::vector<VkDescriptorSet> sets(count);
        VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocation.descriptorPool = mesh.sampler_pools.back();
        allocation.descriptorSetCount = count;
        allocation.pSetLayouts = layouts.data();
        result = check_vk(vkAllocateDescriptorSets(this->context->device, &allocation, sets.data()),
            "allocate sampler descriptor sets");
        if (result.is_err()) {
            return result;
        }
        for (std::uint32_t index = 0; index < count; ++index) {
            const auto& material = mesh.materials[index];
            std::array<VkCopyDescriptorSet, DESCRIPTOR_COUNT> copies{};
            for (std::uint32_t binding = 0; binding < DESCRIPTOR_COUNT; ++binding) {
                copies[binding] = {VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET};
                copies[binding].srcSet = material.descriptors;
                copies[binding].srcBinding = binding;
                copies[binding].dstSet = sets[index];
                copies[binding].dstBinding = binding;
                copies[binding].descriptorCount = 1;
            }
            vkUpdateDescriptorSets(
                this->context->device, 0, nullptr, DESCRIPTOR_COUNT, copies.data());
            std::array<VkDescriptorImageInfo, TEXTURE_COUNT> images{};
            std::array<VkWriteDescriptorSet, TEXTURE_COUNT> writes{};
            for (std::uint32_t texture = 0; texture < TEXTURE_COUNT; ++texture) {
                images[texture] = {this->state->sampler,
                    material.textures[texture]->view,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                writes[texture] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                writes[texture].dstSet = sets[index];
                writes[texture].dstBinding = UNIFORM_COUNT + texture;
                writes[texture].descriptorCount = 1;
                writes[texture].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writes[texture].pImageInfo = &images[texture];
            }
            vkUpdateDescriptorSets(this->context->device, TEXTURE_COUNT, writes.data(), 0, nullptr);
        }
        mesh.sampler_descriptors.emplace(this->state->sampler, std::move(sets));
        return {};
    }
    void VulkanRenderer::present() const {
        auto result = this->context->end_frame();
        if (result.is_err()) {
            fail(*this->state, result.unwrap_err().get_message());
        }
    }
} // namespace enishi::renderer::vulkan
