#include "opengl40_commands.h"
#include <algorithm>
#include <cstdint>
#include <glad/gl.h>
#include <renderer/opengl/opengl40/shader/opengl40_shaders.h>

namespace enishi::renderer::opengl {
    namespace {
        constexpr std::uint32_t NO_INDEX_STRIDE = 0;
        constexpr std::uint32_t MINIMUM_INSTANCE_COUNT = 1;
    } // namespace
    void OpenGL40Commands::submit_command_mesh(OpenGL40RendererState& runtime,
        ResourceAccessor& resource_accessor,
        const types::DrawCommand& command,
        const types::RenderHandle&) {
        if (!runtime.active_framebuffer_complete ||
            command.sub_command != types::SubCommand::Bind) {
            return;
        }
        const auto object = runtime.objects.find(command.handle);
        if (object == runtime.objects.end()) {
            return;
        }
        glBindVertexArray(object->second);
        const auto index_type = runtime.index_types.find(command.handle);
        runtime.active_index_type =
            index_type == runtime.index_types.end() ? GL_UNSIGNED_INT : index_type->second;
        const auto buffers = runtime.mesh_uniform_buffers.find(command.handle);
        if (buffers == runtime.mesh_uniform_buffers.end()) {
            return;
        }
        const auto accessor = resource_accessor.get_buffer_accessor();
        for (const auto& buffer_handle : buffers->second) {
            const auto updater = accessor->get_buffer(buffer_handle);
            if (updater.is_some() && updater.unwrap()) {
                updater.unwrap()->on_update();
            }
        }
        if (!OpenGL40Shaders::use_active_program(runtime)) {
            return;
        }
        const auto bindings = runtime.mesh_draw_bindings.find(command.handle);
        if (bindings == runtime.mesh_draw_bindings.end()) {
            return;
        }
        const auto texture_bindings = runtime.mesh_texture_bindings.find(command.handle);
        const auto material_uniforms = runtime.material_uniform_buffers.find(command.handle);
        const auto index_stride = runtime.index_strides.find(command.handle);
        const auto stride =
            index_stride == runtime.index_strides.end() ? NO_INDEX_STRIDE : index_stride->second;
        for (std::size_t material_index = 0; material_index < bindings->second.size();
             ++material_index) {
            if (material_uniforms != runtime.material_uniform_buffers.end() &&
                material_index < material_uniforms->second.size()) {
                for (const auto buffer : material_uniforms->second[material_index]) {
                    const auto updater = accessor->get_buffer(buffer);
                    if (updater.is_some() && updater.unwrap() != nullptr) {
                        updater.unwrap()->on_update();
                    }
                }
            }
            if (texture_bindings != runtime.mesh_texture_bindings.end() &&
                material_index < texture_bindings->second.size()) {
                for (const auto& [unit, texture] : texture_bindings->second[material_index]) {
                    glActiveTexture(GL_TEXTURE0 + unit);
                    glBindTexture(GL_TEXTURE_2D, texture);
                    glBindSampler(unit, runtime.active_sampler);
                }
            }
            const auto& binding = bindings->second[material_index];
            if (const auto indexed = std::get_if<types::DrawIndexedParameter>(&binding.parameter)) {
                glDrawElementsInstancedBaseVertex(runtime.topology,
                    indexed->index_count,
                    runtime.active_index_type,
                    reinterpret_cast<const void*>(
                        static_cast<std::uintptr_t>(indexed->first_index) * stride),
                    std::max(MINIMUM_INSTANCE_COUNT, indexed->instance_count),
                    static_cast<GLint>(indexed->vertex_offset));

            } else if (const auto plain = std::get_if<types::DrawParameter>(&binding.parameter)) {
                glDrawArraysInstanced(runtime.topology,
                    plain->first_vertex,
                    plain->vertex_count,
                    std::max(MINIMUM_INSTANCE_COUNT, plain->instance_count));
            }
        }
        glActiveTexture(GL_TEXTURE0);
    }
    void OpenGL40Commands::submit_command_topology(
        OpenGL40RendererState& runtime, const types::DrawCommand& command) {
        if (command.sub_command != types::SubCommand::Bind) {
            return;
        }
        const auto value = static_cast<types::PrimitiveTopology>(command.handle.id.handle_id);
        if (value == types::PrimitiveTopology::LineList) {
            runtime.topology = GL_LINES;
        } else if (value == types::PrimitiveTopology::PointList) {
            runtime.topology = GL_POINTS;
        } else {
            runtime.topology = GL_TRIANGLES;
        }
    }
    void OpenGL40Commands::submit_command_image(
        OpenGL40RendererState& runtime, const types::DrawCommand& command) {
        if (command.sub_command != types::SubCommand::Bind) {
            return;
        }
        const auto object = runtime.objects.find(command.handle);
        if (object == runtime.objects.end()) {
            return;
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, object->second);
    }
    void OpenGL40Commands::draw(OpenGL40RendererState& runtime, const types::RenderHandle& handle) {
        const auto binding = runtime.draw_bindings.find(handle);
        if (binding == runtime.draw_bindings.end() ||
            !OpenGL40Shaders::use_active_program(runtime)) {
            return;
        }
        if (const auto indexed =
                std::get_if<types::DrawIndexedParameter>(&binding->second.parameter)) {
            glDrawElementsInstancedBaseVertex(runtime.topology,
                indexed->index_count,
                runtime.active_index_type,
                reinterpret_cast<const void*>(static_cast<std::uintptr_t>(indexed->first_index)),
                indexed->instance_count,
                static_cast<GLint>(indexed->vertex_offset));
        } else if (const auto plain =
                       std::get_if<types::DrawParameter>(&binding->second.parameter)) {
            glDrawArraysInstanced(
                runtime.topology, plain->first_vertex, plain->vertex_count, plain->instance_count);
        }
    }
} // namespace enishi::renderer::opengl
