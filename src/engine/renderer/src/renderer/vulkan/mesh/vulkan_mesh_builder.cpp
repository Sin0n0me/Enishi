#include <array>
#include <cstring>
#include <engine_types/renderer/uniform_buffer/bones.h>
#include <engine_types/renderer/uniform_buffer/material.h>
#include <renderer/common/converter/model_to_mesh.h>
#include <renderer/common/converter/skinned_vertices.h>
#include <renderer/common/vertex_buffer_updater.h>
#include <renderer/vulkan/mesh/vulkan_mesh_builder.h>
#include <renderer/vulkan/runtime/vulkan_handles.h>

namespace enishi::renderer::vulkan {
    namespace {
        std::shared_ptr<VertexBufferUpdater> mapped_updater(
            types::OwnedRenderData&& data, std::shared_ptr<VulkanBuffer> buffer) {
            return std::make_shared<VertexBufferUpdater>(
                std::move(data), [buffer = std::move(buffer)](const types::RenderData& value) {
                    std::memcpy(buffer->mapped, value.raw_data(), value.byte_width());
                });
        }
        platform::RenderResult<void> ensure_model_buffers(types::MeshData& data) {
            if (data.vertices.get_render_data().stride != sizeof(SkinnedVertex)) {
                return foundation::Error(platform::RenderError::MakeError,
                    "Vulkan model meshes require the shared SkinnedVertex layout");
            }
            if (!data.uniforms.contains("Bones")) {
                data.uniforms.emplace("Bones",
                    types::OwnedRenderData{
                        std::vector<glm::mat4>(types::HeavyModelBones::CAPACITY, glm::mat4{1.0f})});
            }
            const auto bones = data.uniforms.at("Bones").get_render_data();
            if (bones.byte_width() > sizeof(types::HeavyModelBones) ||
                bones.byte_width() % sizeof(glm::mat4) != 0) {
                return foundation::Error(platform::RenderError::MakeError,
                    "Vulkan model shader requires a palette of at most 1024 matrices");
            }
            // The descriptor must cover the complete shader array, including unused identities.
            std::vector<glm::mat4> palette(types::HeavyModelBones::CAPACITY, glm::mat4{1.0f});
            std::memcpy(palette.data(), bones.raw_data(), bones.byte_width());
            data.uniforms.at("Bones") = types::OwnedRenderData{palette};
            return {};
        }
    } // namespace
    platform::RenderResult<types::RenderHandle> VulkanMeshBuilder::create_mesh(
        VulkanRendererState& state,
        ResourceAccessor& resource_accessor,
        RenderHandleMapper& handle_mapper,
        const types::ModelData& model,
        const std::vector<types::RenderHandle>&) {
        auto converted = ModelToMesh::to_mesh_data(model);
        if (converted.is_err()) {
            return converted.propagation(platform::RenderError::MakeError);
        }
        auto data = std::move(converted.unwrap_mut());
        auto checked = ensure_model_buffers(data);
        if (checked.is_err()) {
            return std::move(checked).take_err();
        }
        auto mesh = std::make_shared<VulkanMesh>();
        mesh->owner = state.context;
        const auto vertex_data = data.vertices.get_render_data();
        auto vertices =
            state.context->make_buffer(vertex_data.byte_width(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        if (vertices.is_err()) {
            return std::move(vertices).take_err();
        }
        mesh->vertices = vertices.unwrap();
        std::memcpy(mesh->vertices->mapped, vertex_data.raw_data(), vertex_data.byte_width());
        const auto original_indices = data.indices.get_render_data();
        constexpr std::uint32_t BYTE_INDEX_STRIDE = 1;
        constexpr std::uint32_t SHORT_INDEX_STRIDE = 2;
        if (original_indices.stride == BYTE_INDEX_STRIDE) {
            std::vector<std::uint16_t> indices;
            for (auto index : original_indices.bytes) {
                indices.push_back(std::to_integer<std::uint8_t>(index));
            }
            data.indices = types::OwnedRenderData{indices};
        }
        const auto index_data = data.indices.get_render_data();
        mesh->index_type =
            index_data.stride == SHORT_INDEX_STRIDE ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
        auto indices =
            state.context->make_buffer(index_data.byte_width(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        if (indices.is_err()) {
            return std::move(indices).take_err();
        }
        mesh->indices = indices.unwrap();
        std::memcpy(mesh->indices->mapped, index_data.raw_data(), index_data.byte_width());
        auto [resource, handles] = resource_accessor.make_mesh_handles();
        checked = VulkanMeshBuilder::make_mesh_uniforms(
            state, resource_accessor, data.uniforms, handles.uniform_buffers, mesh->uniforms);
        if (checked.is_err()) {
            return std::move(checked).take_err();
        }
        checked = VulkanMeshBuilder::make_materials(state, resource_accessor, data, *mesh, handles);
        if (checked.is_err()) {
            return std::move(checked).take_err();
        }
        const auto [position, updater] = resource_accessor.make_buffer();
        updater = mapped_updater(std::move(data.vertices), mesh->vertices);
        handles.positions =
            types::MeshHandles::PositionStream{position, offsetof(SkinnedVertex, position)};
        handles.uvs = skinned_uv_streams(position);
        const auto handle = VulkanHandles::make(handle_mapper, types::RenderHandleType::Mesh);
        handle_mapper[handle].resource = resource;
        state.meshes.emplace(handle, std::move(mesh));
        return handle;
    }
    platform::RenderResult<void> VulkanMeshBuilder::make_mesh_uniforms(VulkanRendererState& state,
        ResourceAccessor& resource_accessor,
        types::MeshData::UniformMap& uniforms,
        types::MeshHandles::UniformBuffers& named,
        VulkanUniformMap& buffers) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(state.context->physical, &properties);
        for (auto& [name, uniform] : uniforms) {
            const auto data = uniform.get_render_data();
            if (data.byte_width() > properties.limits.maxUniformBufferRange) {
                return foundation::Error(platform::RenderError::MakeError,
                    "Model uniform exceeds Vulkan device limit: " + name);
            }
            auto result =
                state.context->make_buffer(data.byte_width(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
            if (result.is_err()) {
                return std::move(result).take_err();
            }
            auto buffer = result.unwrap();
            std::memcpy(buffer->mapped, data.raw_data(), data.byte_width());
            const auto [handle, stored] = resource_accessor.make_buffer();
            stored = mapped_updater(std::move(uniform), buffer);
            named[name].push_back(handle);
            buffers.emplace(name, std::move(buffer));
        }
        return {};
    }
    platform::RenderResult<void> VulkanMeshBuilder::make_materials(VulkanRendererState& state,
        ResourceAccessor& resource_accessor,
        types::MeshData& data,
        VulkanMesh& mesh,
        types::MeshHandles& handles) {
        constexpr std::uint32_t UNIFORM_COUNT = 3;
        constexpr std::uint32_t TEXTURE_COUNT = 3;
        const auto count = static_cast<std::uint32_t>(data.materials.size());
        const std::array<VkDescriptorPoolSize, 2> sizes{
            {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count * (UNIFORM_COUNT + 1)},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, count * TEXTURE_COUNT}}};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool.maxSets = count;
        pool.poolSizeCount = static_cast<std::uint32_t>(sizes.size());
        pool.pPoolSizes = sizes.data();
        auto result =
            check_vk(vkCreateDescriptorPool(state.context->device, &pool, nullptr, &mesh.pool),
                "vkCreateDescriptorPool");
        if (result.is_err()) {
            return result;
        }
        std::unordered_map<const types::TextureData*, std::shared_ptr<VulkanImage>> textures;
        for (auto& material : data.materials) {
            auto& target = mesh.materials.emplace_back();
            target.draw = material.draw_binding;
            auto& named = handles.material_uniform_buffers.emplace_back();
            if (material.uniforms == nullptr || !material.uniforms->contains("Material")) {
                return foundation::Error(
                    platform::RenderError::MakeError, "Vulkan model requires a Material uniform");
            }
            result = VulkanMeshBuilder::make_mesh_uniforms(
                state, resource_accessor, *material.uniforms, named, target.uniforms);
            if (result.is_err()) {
                return result;
            }
            VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            allocation.descriptorPool = mesh.pool;
            allocation.descriptorSetCount = 1;
            allocation.pSetLayouts = &state.descriptors;
            result = check_vk(
                vkAllocateDescriptorSets(state.context->device, &allocation, &target.descriptors),
                "vkAllocateDescriptorSets");
            if (result.is_err()) {
                return result;
            }
            const std::array<std::string, UNIFORM_COUNT> uniform_names{
                "Camera", "Bones", "Material"};
            std::array<VkDescriptorBufferInfo, UNIFORM_COUNT> buffer_infos{};
            std::array<VkWriteDescriptorSet, UNIFORM_COUNT + TEXTURE_COUNT> writes{};
            for (std::uint32_t binding = 0; binding < UNIFORM_COUNT; ++binding) {
                const auto& name = uniform_names[binding];
                auto& uniforms = target.uniforms.contains(name) ? target.uniforms : mesh.uniforms;
                if (!uniforms.contains(name)) {
                    return foundation::Error(
                        platform::RenderError::MakeError, "Missing Vulkan model uniform: " + name);
                }
                const auto& buffer = uniforms.at(name);
                buffer_infos[binding] = {buffer->buffer, 0, buffer->size};
                writes[binding] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                writes[binding].dstSet = target.descriptors;
                writes[binding].dstBinding = binding;
                writes[binding].descriptorCount = 1;
                writes[binding].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                writes[binding].pBufferInfo = &buffer_infos[binding];
            }
            const std::array<std::string, TEXTURE_COUNT> texture_names{
                "model_texture", "sphere_texture", "toon_texture"};
            std::array<VkDescriptorImageInfo, TEXTURE_COUNT> image_infos{};
            for (std::uint32_t index = 0; index < TEXTURE_COUNT; ++index) {
                const auto found = material.textures.find(texture_names[index]);
                if (found == material.textures.end() || found->second == nullptr) {
                    return foundation::Error(platform::RenderError::MakeError,
                        "Missing Vulkan model texture: " + texture_names[index]);
                }
                auto cached = textures.find(found->second.get());
                if (cached == textures.end()) {
                    auto image = state.context->make_texture(*found->second);
                    if (image.is_err()) {
                        return std::move(image).take_err();
                    }
                    cached = textures.emplace(found->second.get(), image.unwrap()).first;
                }
                target.textures.push_back(cached->second);
                image_infos[index] = {
                    state.sampler, cached->second->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                const auto binding = UNIFORM_COUNT + index;
                writes[binding] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                writes[binding].dstSet = target.descriptors;
                writes[binding].dstBinding = binding;
                writes[binding].descriptorCount = 1;
                writes[binding].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writes[binding].pImageInfo = &image_infos[index];
            }
            constexpr std::uint32_t SHADOW_BINDING = 6;
            const auto& shadow_buffer = mesh.uniforms.at("ShadowMap");
            const VkDescriptorBufferInfo shadow_info{shadow_buffer->buffer, 0, shadow_buffer->size};
            VkWriteDescriptorSet shadow_write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            shadow_write.dstSet = target.descriptors;
            shadow_write.dstBinding = SHADOW_BINDING;
            shadow_write.descriptorCount = 1;
            shadow_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            shadow_write.pBufferInfo = &shadow_info;
            vkUpdateDescriptorSets(state.context->device, 1, &shadow_write, 0, nullptr);
            vkUpdateDescriptorSets(state.context->device,
                static_cast<std::uint32_t>(writes.size()),
                writes.data(),
                0,
                nullptr);
            mesh.sampler_descriptors[state.sampler].push_back(target.descriptors);
        }
        return {};
    }
} // namespace enishi::renderer::vulkan
