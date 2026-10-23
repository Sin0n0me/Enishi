#pragma once
#include <engine_types/renderer/mesh/mesh_data.h>
#include <renderer/common/resource_accessor.h>
#include <renderer/opengl/opengl40/runtime/opengl40_renderer_state.h>

namespace enishi::renderer::opengl {
    class OpenGL40UniformBuffers final {
      public:
        static platform::RenderResult<std::vector<types::HandleId>> make(
            OpenGL40RendererState& runtime,
            ResourceAccessor& resource_accessor,
            const std::vector<types::RenderHandle>& shader_reflections,
            types::MeshData::UniformMap& uniforms,
            types::MeshHandles::UniformBuffers& named_uniforms);
    };
} // namespace enishi::renderer::opengl
