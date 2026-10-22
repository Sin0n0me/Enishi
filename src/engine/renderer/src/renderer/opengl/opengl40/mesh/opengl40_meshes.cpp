#include "opengl40_meshes.h"
#include <cstddef>
#include <cstdint>
#include <engine_types/assets/model/model_data.h>
#include <glad/gl.h>
#include <renderer/common/converter/model_to_mesh.h>
#include <renderer/common/converter/skinned_vertices.h>
#include <renderer/common/vertex_buffer_updater.h>
#include <renderer/opengl/opengl40/buffer/opengl40_buffers.h>
#include <renderer/opengl/opengl40/image/opengl40_images.h>
#include <renderer/opengl/opengl40/runtime/opengl40_handles.h>
#include <renderer/opengl/opengl40/support/opengl40_errors.h>

namespace enishi::renderer::opengl {
    namespace {
        constexpr std::uint32_t NO_GL_OBJECT = 0;
        constexpr std::uint32_t RESOURCE_COUNT = 1;
        constexpr std::uint32_t POSITION_ATTRIBUTE = 0;
        constexpr std::uint32_t NORMAL_ATTRIBUTE = 1;
        constexpr std::uint32_t TEXTURE_COORDINATE_ATTRIBUTE = 2;
        constexpr std::uint32_t BLEND_INDEX_ATTRIBUTE = 3;
        constexpr std::uint32_t BLEND_WEIGHT_ATTRIBUTE = 4;
        constexpr std::int32_t POSITION_COMPONENT_COUNT = 3;
        constexpr std::int32_t NORMAL_COMPONENT_COUNT = 3;
        constexpr std::int32_t TEXTURE_COORDINATE_COMPONENT_COUNT = 2;
        constexpr std::int32_t BLEND_INDEX_COMPONENT_COUNT = 4;
        constexpr std::int32_t BLEND_WEIGHT_COMPONENT_COUNT = 4;
        constexpr std::uint32_t SKINNING_METHOD_ATTRIBUTE = 5;
        constexpr std::uint32_t BLEND_CENTER_ATTRIBUTE = 6;
        constexpr std::uint32_t BLEND_ANCHOR0_ATTRIBUTE = 7;
        constexpr std::uint32_t BLEND_ANCHOR1_ATTRIBUTE = 8;
        constexpr std::int32_t METHOD_COMPONENT_COUNT = 1;
        constexpr std::size_t TEXTURE_COORDINATE_OFFSET_MULTIPLIER = 2;
        constexpr std::uint32_t BYTE_INDEX_STRIDE = 1;
        constexpr std::uint32_t SHORT_INDEX_STRIDE = 2;
        constexpr std::uint32_t UNSUPPORTED_FIRST_INSTANCE = 0;
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
        for (const auto& material : data.materials) {
            if (const auto indexed =
                    std::get_if<types::DrawIndexedParameter>(&material.draw_binding.parameter)) {
                if (indexed->first_instance != UNSUPPORTED_FIRST_INSTANCE) {
                    return foundation::Error(platform::RenderError::MakeError,
                        "OpenGL 4.0 does not support non-zero first_instance");
                }
            } else if (const auto plain =
                           std::get_if<types::DrawParameter>(&material.draw_binding.parameter)) {
                if (plain->first_instance != UNSUPPORTED_FIRST_INSTANCE) {
                    return foundation::Error(platform::RenderError::MakeError,
                        "OpenGL 4.0 does not support non-zero first_instance");
                }
            }
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
        const auto make_uniforms = [&](types::MeshData::UniformMap& uniforms,
                                       types::MeshHandles::UniformBuffers& named_uniforms)
            -> platform::RenderResult<std::vector<types::HandleId>> {
            std::vector<types::HandleId> uniform_handles;
            for (const auto& reflection_handle : shader_reflections) {
                const auto reflection = runtime.reflections.find(reflection_handle);
                if (reflection == runtime.reflections.end()) {
                    continue;
                }
                const auto* inputs = reflection->second->get_shader_input_reflection();
                for (const auto& resource : inputs->get_input_resources()) {
                    if (resource.type != types::ShaderInputResourceType::UniformBuffer) {
                        continue;
                    }
                    const auto uniform = uniforms.find(resource.name);
                    if (uniform == uniforms.end()) {
                        continue;
                    }
                    const auto render_data = uniform->second.get_render_data();
                    GLuint buffer = NO_GL_OBJECT;
                    glGenBuffers(RESOURCE_COUNT, &buffer);
                    glBindBuffer(GL_UNIFORM_BUFFER, buffer);
                    glBufferData(GL_UNIFORM_BUFFER,
                        static_cast<GLsizeiptr>(render_data.byte_width()),
                        render_data.raw_data(),
                        GL_DYNAMIC_DRAW);
                    const auto binding = runtime.uniform_block_bindings.find(resource.name);
                    if (binding == runtime.uniform_block_bindings.end()) {
                        glDeleteBuffers(RESOURCE_COUNT, &buffer);
                        return foundation::Error(platform::RenderError::MakeError,
                            "Uniform buffer binding was not created for the shader resource");
                    }
                    glBindBufferBase(GL_UNIFORM_BUFFER, binding->second, buffer);
                    auto result = check_gl_errors("uniform buffer allocation and binding");
                    if (result.is_err()) {
                        glDeleteBuffers(RESOURCE_COUNT, &buffer);
                        return std::move(result).take_err().add_message(resource.name);
                    }
                    runtime.buffers.emplace_back(buffer);
                    auto updater = std::make_shared<OpenGLUniformUpdater>(
                        std::move(uniform->second), buffer, binding->second);
                    runtime.uniform_updaters.emplace_back(updater);
                    const auto [buffer_handle, _] = resource_accessor.make_buffer();
                    resource_accessor.add_interface(buffer_handle, updater);
                    uniform_handles.emplace_back(buffer_handle);
                    named_uniforms[resource.name].push_back(buffer_handle);
                    uniforms.erase(uniform);
                }
            }
            return uniform_handles;
        };
        types::MeshHandles::UniformBuffers named_uniforms;
        auto shared_uniforms = make_uniforms(data.uniforms, named_uniforms);
        if (shared_uniforms.is_err()) {
            return shared_uniforms.propagation(platform::RenderError::MakeError);
        }
        std::vector<std::vector<types::HandleId>> material_uniform_handles;
        std::vector<types::MeshHandles::UniformBuffers> material_uniforms;
        for (auto& material : data.materials) {
            auto& named = material_uniforms.emplace_back();
            if (material.uniforms == nullptr) {
                material_uniform_handles.emplace_back();
                continue;
            }
            auto uniforms = make_uniforms(*material.uniforms, named);
            if (uniforms.is_err()) {
                return uniforms.propagation(platform::RenderError::MakeError);
            }
            material_uniform_handles.push_back(std::move(uniforms).unwrap());
        }
        GLuint vao = NO_GL_OBJECT;
        glGenVertexArrays(RESOURCE_COUNT, &vao);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, runtime.objects.at(vertex.unwrap()));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, runtime.objects.at(index.unwrap()));
        const auto stride = static_cast<GLsizei>(data.vertices.get_render_data().stride);
        glEnableVertexAttribArray(POSITION_ATTRIBUTE);
        glVertexAttribPointer(
            POSITION_ATTRIBUTE, POSITION_COMPONENT_COUNT, GL_FLOAT, GL_FALSE, stride, nullptr);
        if (stride >= static_cast<GLsizei>(sizeof(types::Vertex))) {
            glEnableVertexAttribArray(NORMAL_ATTRIBUTE);

            glVertexAttribPointer(NORMAL_ATTRIBUTE,
                NORMAL_COMPONENT_COUNT,
                GL_FLOAT,
                GL_FALSE,
                stride,
                reinterpret_cast<const void*>(sizeof(glm::vec3)));
            glEnableVertexAttribArray(TEXTURE_COORDINATE_ATTRIBUTE);
            glVertexAttribPointer(TEXTURE_COORDINATE_ATTRIBUTE,
                TEXTURE_COORDINATE_COMPONENT_COUNT,
                GL_FLOAT,
                GL_FALSE,
                stride,
                reinterpret_cast<const void*>(
                    sizeof(glm::vec3) * TEXTURE_COORDINATE_OFFSET_MULTIPLIER));
        }
        if (stride == static_cast<GLsizei>(sizeof(SkinnedVertex))) {
            glEnableVertexAttribArray(BLEND_INDEX_ATTRIBUTE);

            glVertexAttribIPointer(BLEND_INDEX_ATTRIBUTE,
                BLEND_INDEX_COMPONENT_COUNT,
                GL_UNSIGNED_INT,
                stride,
                reinterpret_cast<const void*>(offsetof(SkinnedVertex, bones)));
            glEnableVertexAttribArray(BLEND_WEIGHT_ATTRIBUTE);
            glVertexAttribPointer(BLEND_WEIGHT_ATTRIBUTE,
                BLEND_WEIGHT_COMPONENT_COUNT,
                GL_FLOAT,
                GL_FALSE,
                stride,
                reinterpret_cast<const void*>(offsetof(SkinnedVertex, weights)));
            glEnableVertexAttribArray(SKINNING_METHOD_ATTRIBUTE);
            glVertexAttribIPointer(SKINNING_METHOD_ATTRIBUTE,
                METHOD_COMPONENT_COUNT,
                GL_UNSIGNED_INT,
                stride,
                reinterpret_cast<const void*>(offsetof(SkinnedVertex, method)));
            const auto bind_anchor = [stride](GLuint attribute, std::size_t offset) {
                glEnableVertexAttribArray(attribute);
                glVertexAttribPointer(attribute,
                    POSITION_COMPONENT_COUNT,
                    GL_FLOAT,
                    GL_FALSE,
                    stride,
                    reinterpret_cast<const void*>(offset));
            };
            bind_anchor(BLEND_CENTER_ATTRIBUTE, offsetof(SkinnedVertex, center));
            bind_anchor(BLEND_ANCHOR0_ATTRIBUTE, offsetof(SkinnedVertex, anchor0));
            bind_anchor(BLEND_ANCHOR1_ATTRIBUTE, offsetof(SkinnedVertex, anchor1));
        }
        auto layout_result = check_gl_errors("mesh vertex array configuration");
        if (layout_result.is_err()) {
            glDeleteVertexArrays(RESOURCE_COUNT, &vao);
            return std::move(layout_result).take_err();
        }
        runtime.vertex_arrays.emplace_back(vao);
        const auto handle =
            OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::Mesh);
        auto [mesh_resource, mesh_handles] = resource_accessor.make_mesh_handles();
        mesh_handles.uniform_buffers = std::move(named_uniforms);
        mesh_handles.material_uniform_buffers = std::move(material_uniforms);
        const auto vertex_object = runtime.objects.at(vertex.unwrap());
        auto vertex_updater = std::make_shared<VertexBufferUpdater>(
            std::move(data.vertices), [vertex_object](const types::RenderData& vertices) {
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
        mesh_handles.mesh_handles.emplace_back(vertex.unwrap());
        mesh_handles.mesh_handles.emplace_back(index.unwrap());
        handle_mapper[handle].resource = mesh_resource;
        runtime.mesh_uniform_buffers.emplace(handle, std::move(shared_uniforms).unwrap());
        runtime.material_uniform_buffers.emplace(handle, std::move(material_uniform_handles));
        runtime.objects.emplace(handle, vao);
        const auto index_stride = data.indices.get_render_data().stride;
        auto index_type = GL_UNSIGNED_INT;
        if (index_stride == SHORT_INDEX_STRIDE) {
            index_type = GL_UNSIGNED_SHORT;
        } else if (index_stride == BYTE_INDEX_STRIDE) {
            index_type = GL_UNSIGNED_BYTE;
        }
        runtime.index_types.emplace(handle, index_type);
        runtime.index_strides.emplace(handle, data.indices.get_render_data().stride);
        std::vector<types::DrawBinding> draw_bindings;
        std::vector<types::RenderHandle> draw_handles;
        std::vector<std::unordered_map<std::uint32_t, std::uint32_t>> texture_bindings;
        draw_bindings.reserve(data.materials.size());
        draw_handles.reserve(data.materials.size());
        texture_bindings.reserve(data.materials.size());
        std::unordered_map<const types::TextureData*, std::uint32_t> texture_objects;
        for (const auto& material : data.materials) {
            draw_bindings.emplace_back(material.draw_binding);
            std::unordered_map<std::uint32_t, std::uint32_t> material_texture_bindings;
            for (const auto& [name, texture] : material.textures) {
                const auto binding = runtime.sampler_bindings.find(name);
                if (binding == runtime.sampler_bindings.end() || !texture) {
                    continue;
                }
                const auto cached_texture = texture_objects.find(texture.get());
                if (cached_texture != texture_objects.end()) {
                    material_texture_bindings.emplace(binding->second, cached_texture->second);
                    continue;
                }
                const auto created_texture =
                    OpenGL40Images::create_texture(runtime, handle_mapper, *texture);
                if (created_texture.is_err()) {
                    return created_texture.propagation(platform::RenderError::MakeError);
                }
                const auto object = runtime.objects.find(created_texture.unwrap());
                if (object == runtime.objects.end()) {
                    return foundation::Error(
                        platform::RenderError::MakeError, "OpenGL texture object was not created");
                }
                texture_objects.emplace(texture.get(), object->second);
                material_texture_bindings.emplace(binding->second, object->second);
            }
            texture_bindings.emplace_back(std::move(material_texture_bindings));
            const auto draw_handle =
                OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::Draw);
            handle_mapper[draw_handle].resource = handle_mapper[index.unwrap()].resource;
            runtime.draw_bindings.emplace(draw_handle, material.draw_binding);
            draw_handles.emplace_back(draw_handle);
        }
        runtime.mesh_draw_bindings.emplace(handle, std::move(draw_bindings));
        runtime.mesh_texture_bindings.emplace(handle, std::move(texture_bindings));
        for (const auto& draw_handle : draw_handles) {
            mesh_handles.mesh_handles.emplace_back(draw_handle);
        }
        return handle;
    }
} // namespace enishi::renderer::opengl
