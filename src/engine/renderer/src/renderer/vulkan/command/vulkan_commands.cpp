#include <algorithm>
#include <debug/log/logger.h>
#include <renderer/vulkan/command/vulkan_commands.h>
#include <renderer/vulkan/pipeline/vulkan_pipeline.h>

namespace enishi::renderer::vulkan {
    namespace {
        void fail(VulkanRendererState& state, std::string message) {
            if (!state.failed) {
                debug::Logger::error(std::move(message));
            }
            state.failed = true;
        }
    } // namespace
    void VulkanCommands::setup_viewports() {
        // The current drawable extent is applied after acquiring the swapchain image.
    }
    void VulkanCommands::setup_views(
        VulkanRendererState& state, ResourceAccessor& resource_accessor) {
        if (state.failed) {
            return;
        }
        glm::vec4 clear_color{};
        const auto targets = resource_accessor.get_render_target_views();
        if (!targets.empty()) {
            clear_color = targets.front()->get_clear_color();
        }
        state.sampled_shadow = state.fallback_shadow;
        state.active_shadow.reset();
        state.main_viewport = {};
        auto result = state.context->begin_frame(clear_color);
        if (result.is_err()) {
            fail(state, result.unwrap_err().get_message());
        }
    }
    void VulkanCommands::submit_command_shader(
        VulkanRendererState& state, const types::DrawCommand& command) {
        const auto found = state.shaders.find(command.handle);
        if (command.sub_command != types::SubCommand::Bind || found == state.shaders.end()) {
            return;
        }
        if (found->second.kind == types::ShaderKind::Vertex) {
            state.vertex_shader = command.handle;
        }
        if (found->second.kind == types::ShaderKind::Pixel) {
            state.pixel_shader = command.handle;
        }
    }
    void VulkanCommands::submit_command_state(
        VulkanRendererState& state, const types::DrawCommand& command) {
        if (command.sub_command != types::SubCommand::Bind) {
            return;
        }
        if (state.rasterizers.contains(command.handle)) {
            state.rasterizer = command.handle;
        }
        if (state.depths.contains(command.handle)) {
            state.depth = command.handle;
        }
        if (state.blends.contains(command.handle)) {
            state.blend = command.handle;
        }
        if (state.samplers.contains(command.handle)) {
            state.sampler = state.samplers.at(command.handle);
        }
    }
    void VulkanCommands::submit_command_view(VulkanRendererState& state,
        ResourceAccessor& resource_accessor,
        const types::DrawCommand& command,
        const types::RenderHandle& render_target) {
        if (!state.context->recording || state.failed ||
            command.sub_command != types::SubCommand::Bind) {
            return;
        }
        const auto found = state.shadow_targets.find(command.handle);
        if (found == state.shadow_targets.end()) {
            return; // Main attachments are opened lazily, after the shadow pass.
        }
        const auto depth = resource_accessor.get_depth_stencil_view(command.handle.id);
        if (depth.is_some() && !render_target.is_valid()) {
            auto result =
                state.context->begin_shadow_pass(*found->second, depth.unwrap()->clear_depth());
            if (result.is_err()) {
                fail(state, result.unwrap_err().get_message());
                return;
            }
            state.active_shadow = found->second;
        } else if (resource_accessor.get_shader_resource_view(command.handle.id).is_some()) {
            if (state.active_shadow != found->second) {
                fail(state, "Sampled shadow must have been rendered in this frame");
                return;
            }
            state.context->begin_main_pass();
            state.sampled_shadow = found->second;
        }
    }
    void VulkanCommands::submit_command_viewport(
        VulkanRendererState& state, const types::DrawCommand& command) {
        const auto found = state.viewports.find(command.handle);
        if (!state.context->recording || found == state.viewports.end()) {
            return;
        }
        if (!state.context->shadow_recording) {
            state.main_viewport = command.handle;
        }
        const auto& value = found->second;
        VkViewport viewport{value.left_top_x,
            value.left_top_y,
            value.width,
            value.height,
            value.min_depth,
            value.max_depth};
        vkCmdSetViewport(state.context->command, 0, 1, &viewport);
    }
    void VulkanCommands::submit_command_topology(
        VulkanRendererState& state, const types::DrawCommand& command) {
        if (command.handle.id.handle_id !=
            static_cast<std::uint32_t>(types::PrimitiveTopology::TriangleList)) {
            fail(state, "Vulkan model pass requires triangle-list topology");
        }
    }
    void VulkanCommands::submit_command_vertex_layout(const types::DrawCommand&) {
        // Layout was validated against SkinnedVertex during render-pass construction.
    }
    void VulkanCommands::submit_command_buffer(
        VulkanRendererState& state, const types::DrawCommand&) {
        fail(state, "Vulkan buffer commands must be submitted through a model mesh");
    }
    void VulkanCommands::submit_command_image(
        VulkanRendererState& state, const types::DrawCommand&) {
        fail(state, "Vulkan textures must be submitted through material descriptors");
    }
    void VulkanCommands::draw(VulkanRendererState& state, const types::RenderHandle&) {
        fail(state, "Vulkan draws must be submitted through a model mesh");
    }
    void VulkanCommands::submit_command_mesh(
        VulkanRendererState& state, const types::DrawCommand& command, const types::RenderHandle&) {
        if (!state.context->recording || state.failed ||
            command.sub_command != types::SubCommand::Bind) {
            return;
        }
        const auto found = state.meshes.find(command.handle);
        if (found == state.meshes.end()) {
            fail(state, "Invalid Vulkan mesh handle");
            return;
        }
        if (!state.context->shadow_recording) {
            state.context->begin_main_pass();
            // Opening the main pass restores its extent; preserve an explicitly submitted viewport.
            if (state.main_viewport.is_valid()) {
                VulkanCommands::submit_command_viewport(
                    state, {.handle = state.main_viewport, .sub_command = types::SubCommand::Bind});
            }
        }
        auto pipeline = VulkanPipeline::active_pipeline(state);
        if (pipeline.is_err()) {
            fail(state, pipeline.unwrap_err().get_message());
            return;
        }
        auto& mesh = *found->second;
        auto prepared = VulkanCommands::prepare_mesh_sampler(state, mesh);
        if (prepared.is_err()) {
            fail(state, prepared.unwrap_err().get_message());
            return;
        }
        const auto& descriptors = mesh.sampler_descriptors.at(state.sampler);
        const auto buffer = state.context->command;
        vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.unwrap());
        const auto& shadow =
            state.context->shadow_recording ? state.fallback_shadow : state.sampled_shadow;
        constexpr std::uint32_t SHADOW_SET = 1;
        vkCmdBindDescriptorSets(buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            state.pipeline_layout,
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
                state.pipeline_layout,
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
    void VulkanCommands::present(VulkanRendererState& state) {
        auto result = state.context->end_frame();
        if (result.is_err()) {
            fail(state, result.unwrap_err().get_message());
        }
    }
    platform::RenderResult<void> VulkanCommands::prepare_mesh_sampler(
        VulkanRendererState& state, VulkanMesh& mesh) {
        if (mesh.sampler_descriptors.contains(state.sampler)) {
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
                         state.context->device, &pool, nullptr, &mesh.sampler_pools.back()),
                "create sampler descriptor pool");
        if (result.is_err()) {
            return result;
        }
        std::vector<VkDescriptorSetLayout> layouts(count, state.descriptors);
        std::vector<VkDescriptorSet> sets(count);
        VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocation.descriptorPool = mesh.sampler_pools.back();
        allocation.descriptorSetCount = count;
        allocation.pSetLayouts = layouts.data();
        result = check_vk(vkAllocateDescriptorSets(state.context->device, &allocation, sets.data()),
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
                state.context->device, 0, nullptr, DESCRIPTOR_COUNT, copies.data());
            std::array<VkDescriptorImageInfo, TEXTURE_COUNT> images{};
            std::array<VkWriteDescriptorSet, TEXTURE_COUNT> writes{};
            for (std::uint32_t texture = 0; texture < TEXTURE_COUNT; ++texture) {
                images[texture] = {state.sampler,
                    material.textures[texture]->view,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                writes[texture] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                writes[texture].dstSet = sets[index];
                writes[texture].dstBinding = UNIFORM_COUNT + texture;
                writes[texture].descriptorCount = 1;
                writes[texture].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writes[texture].pImageInfo = &images[texture];
            }
            vkUpdateDescriptorSets(state.context->device, TEXTURE_COUNT, writes.data(), 0, nullptr);
        }
        mesh.sampler_descriptors.emplace(state.sampler, std::move(sets));
        return {};
    }
} // namespace enishi::renderer::vulkan
