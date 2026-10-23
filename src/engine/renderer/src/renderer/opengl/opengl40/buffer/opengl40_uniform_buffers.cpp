#include "opengl40_uniform_buffers.h"
#include <renderer/opengl/opengl40/support/opengl40_errors.h>
namespace enishi::renderer::opengl {
    namespace {
        constexpr std::uint32_t NO_GL_OBJECT = 0;
        constexpr std::uint32_t RESOURCE_COUNT = 1;
    } // namespace
    platform::RenderResult<std::vector<types::HandleId>> OpenGL40UniformBuffers::make(
        OpenGL40RendererState& runtime,
        ResourceAccessor& resource_accessor,
        const std::vector<types::RenderHandle>& shader_reflections,
        types::MeshData::UniformMap& uniforms,
        types::MeshHandles::UniformBuffers& named_uniforms) {
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
    }
} // namespace enishi::renderer::opengl
