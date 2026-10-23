#include "opengl40_meshes.h"
#include <cstddef>
#include <cstdint>
#include <engine_types/assets/model/model_data.h>
#include <glad/gl.h>
#include <renderer/common/converter/model_to_mesh.h>
#include <renderer/common/converter/skinned_vertices.h>
#include <renderer/common/vertex_buffer_updater.h>
#include <renderer/opengl/opengl40/buffer/opengl40_buffers.h>
#include <renderer/opengl/opengl40/buffer/opengl40_uniform_buffers.h>
#include <renderer/opengl/opengl40/mesh/opengl40_materials.h>
#include <renderer/opengl/opengl40/mesh/opengl40_vertex_arrays.h>
#include <renderer/opengl/opengl40/runtime/opengl40_handles.h>
#include <renderer/opengl/opengl40/support/opengl40_errors.h>

namespace enishi::renderer::opengl {
    namespace {
        constexpr std::uint32_t BYTE_INDEX_STRIDE = 1;
        constexpr std::uint32_t SHORT_INDEX_STRIDE = 2;
    } // namespace
    platform::RenderResult<types::RenderHandle> OpenGL40Meshes::create_mesh(
        OpenGL40RendererState& runtime,
        ResourceAccessor& resource_accessor,
        RenderHandleMapper& handle_mapper,
        const types::ModelData& model,
        const std::vector<types::RenderHandle>& shader_reflections) {
        auto pending = check_gl_errors("before mesh creation");
        if (pending.is_err()) {
            return std::move(pending).take_err();
        }
        auto mesh = ModelToMesh::to_mesh_data(model);
        if (mesh.is_err()) {
            return mesh.propagation(platform::RenderError::MakeError);
        }
        auto data = std::move(mesh.unwrap_mut());
        auto valid = OpenGL40Materials::validate(data.materials);
        if (valid.is_err()) {
            return std::move(valid).take_err();
        }
        auto vertex = OpenGL40Buffers::make_buffer(
            runtime, handle_mapper, data.vertices.get_render_data(), GL_ARRAY_BUFFER);
        if (vertex.is_err()) {
            return std::move(vertex);
        }
        auto index = OpenGL40Buffers::make_buffer(
            runtime, handle_mapper, data.indices.get_render_data(), GL_ELEMENT_ARRAY_BUFFER);
        if (index.is_err()) {
            return std::move(index);
        }
        types::MeshHandles::UniformBuffers named_uniforms;
        auto shared_uniforms = OpenGL40UniformBuffers::make(
            runtime, resource_accessor, shader_reflections, data.uniforms, named_uniforms);
        if (shared_uniforms.is_err()) {
            return shared_uniforms.propagation(platform::RenderError::MakeError);
        }
        std::vector<types::MeshHandles::UniformBuffers> material_uniforms;
        auto material_uniform_handles = OpenGL40Materials::make_uniforms(
            runtime, resource_accessor, shader_reflections, data.materials, material_uniforms);
        if (material_uniform_handles.is_err()) {
            return material_uniform_handles.propagation(platform::RenderError::MakeError);
        }
        auto vao = OpenGL40VertexArrays::create(runtime,
            runtime.objects.at(vertex.unwrap()),
            runtime.objects.at(index.unwrap()),
            static_cast<GLsizei>(data.vertices.get_render_data().stride));
        if (vao.is_err()) {
            return std::move(vao).take_err();
        }
        const auto handle =
            OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::Mesh);
        auto [mesh_resource, mesh_handles] = resource_accessor.make_mesh_handles();
        mesh_handles.uniform_buffers = std::move(named_uniforms);
        mesh_handles.material_uniform_buffers = std::move(material_uniforms);
        OpenGL40Meshes::register_vertex_stream(resource_accessor,
            runtime.objects.at(vertex.unwrap()),
            std::move(data.vertices),
            mesh_handles);
        mesh_handles.mesh_handles.emplace_back(vertex.unwrap());
        mesh_handles.mesh_handles.emplace_back(index.unwrap());
        handle_mapper[handle].resource = mesh_resource;
        runtime.mesh_uniform_buffers.emplace(handle, std::move(shared_uniforms).unwrap());
        runtime.material_uniform_buffers.emplace(
            handle, std::move(material_uniform_handles).unwrap());
        runtime.objects.emplace(handle, vao.unwrap());
        const auto index_stride = data.indices.get_render_data().stride;
        auto index_type = GL_UNSIGNED_INT;
        if (index_stride == SHORT_INDEX_STRIDE) {
            index_type = GL_UNSIGNED_SHORT;
        } else if (index_stride == BYTE_INDEX_STRIDE) {
            index_type = GL_UNSIGNED_BYTE;
        }
        runtime.index_types.emplace(handle, index_type);
        runtime.index_strides.emplace(handle, data.indices.get_render_data().stride);
        auto materials = OpenGL40Materials::make_bindings(
            runtime, handle_mapper, data, handle, index.unwrap(), mesh_handles);
        if (materials.is_err()) {
            return std::move(materials).take_err();
        }
        return handle;
    }
    void OpenGL40Meshes::register_vertex_stream(ResourceAccessor& resource_accessor,
        GLuint vertex_object,
        types::OwnedRenderData&& vertices,
        types::MeshHandles& mesh_handles) {
        auto vertex_updater = std::make_shared<VertexBufferUpdater>(
            std::move(vertices), [vertex_object](const types::RenderData& vertices) {
                constexpr GLintptr DATA_OFFSET = 0;
                glBindBuffer(GL_ARRAY_BUFFER, vertex_object);
                glBufferSubData(GL_ARRAY_BUFFER,
                    DATA_OFFSET,
                    static_cast<GLsizeiptr>(vertices.byte_width()),
                    vertices.raw_data());
            });
        const auto [position_handle, position_buffer] = resource_accessor.make_buffer();
        position_buffer = std::move(vertex_updater);
        mesh_handles.positions =
            types::MeshHandles::PositionStream{position_handle, offsetof(SkinnedVertex, position)};
        mesh_handles.uvs = skinned_uv_streams(position_handle);
    }
} // namespace enishi::renderer::opengl
