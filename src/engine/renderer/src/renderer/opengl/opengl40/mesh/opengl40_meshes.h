#pragma once
#include <glad/gl.h>
#include <renderer/common/render_handle_mapper.h>
#include <renderer/common/resource_accessor.h>
#include <renderer/opengl/opengl40/runtime/opengl40_renderer_state.h>

namespace enishi::renderer::opengl {
    class OpenGL40Meshes final {
      public:
        static platform::RenderResult<types::RenderHandle> create_mesh(
            OpenGL40RendererState& runtime,
            ResourceAccessor& resource_accessor,
            RenderHandleMapper& handle_mapper,
            const types::ModelData& model,
            const std::vector<types::RenderHandle>& shader_reflections);

      private:
        static void register_vertex_stream(ResourceAccessor& resource_accessor,
            GLuint vertex_object,
            types::OwnedRenderData&& vertices,
            types::MeshHandles& mesh_handles);
    };
} // namespace enishi::renderer::opengl
