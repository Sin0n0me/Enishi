#pragma once
#include <renderer/common/render_handle_mapper.h>
#include <renderer/common/resource_accessor.h>
#include <renderer/vulkan/runtime/vulkan_renderer_state.h>
namespace enishi::renderer::vulkan {
    class VulkanMeshBuilder final {
      public:
        static platform::RenderResult<types::RenderHandle> create_mesh(VulkanRendererState& state,
            ResourceAccessor& resource_accessor,
            RenderHandleMapper& handle_mapper,
            const types::ModelData& model,
            const std::vector<types::RenderHandle>&);

      private:
        static platform::RenderResult<void> make_mesh_uniforms(VulkanRendererState& state,
            ResourceAccessor& resource_accessor,
            types::MeshData::UniformMap& uniforms,
            types::MeshHandles::UniformBuffers& named,
            VulkanUniformMap& buffers);
        static platform::RenderResult<void> make_materials(VulkanRendererState& state,
            ResourceAccessor& resource_accessor,
            types::MeshData& data,
            VulkanMesh& mesh,
            types::MeshHandles& handles);
    };
} // namespace enishi::renderer::vulkan
