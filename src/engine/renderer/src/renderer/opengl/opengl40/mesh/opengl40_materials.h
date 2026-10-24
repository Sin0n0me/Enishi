#pragma once
#include <engine_types/renderer/mesh/mesh_data.h>
#include <renderer/common/render_handle_mapper.h>
#include <renderer/common/resource_accessor.h>
#include <renderer/opengl/opengl40/runtime/opengl40_renderer_state.h>

namespace enishi::renderer::opengl {
    class OpenGL40Materials final {
      public:
        static platform::RenderResult<std::vector<std::vector<types::HandleId>>> make_uniforms(
            OpenGL40RendererState& runtime,
            ResourceAccessor& resource_accessor,
            const std::vector<types::RenderHandle>& shader_reflections,
            std::vector<types::MeshMaterial>& materials,
            std::vector<types::MeshHandles::UniformBuffers>& material_uniforms);
        static platform::RenderResult<void> make_bindings(OpenGL40RendererState& runtime,
            RenderHandleMapper& handle_mapper,
            const types::MeshData& data,
            types::RenderHandle handle,
            types::RenderHandle index,
            types::MeshHandles& mesh_handles);
        static platform::RenderResult<void> validate(
            const std::vector<types::MeshMaterial>& materials);
    };
} // namespace enishi::renderer::opengl
