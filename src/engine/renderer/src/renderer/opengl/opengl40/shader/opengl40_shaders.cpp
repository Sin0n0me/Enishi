#include "opengl40_shaders.h"
#include <algorithm>
#include <cstdint>
#include <glad/gl.h>
#include <limits>
#include <renderer/opengl/opengl40/runtime/opengl40_handles.h>
#include <renderer/opengl/opengl40/support/opengl40_errors.h>
#include <unordered_set>

namespace enishi::renderer::opengl {
    namespace {
        constexpr std::uint32_t NO_GL_OBJECT = 0;
        constexpr std::uint32_t SHADOW_TEXTURE_UNIT = 3;
        constexpr std::uint32_t SHADER_SOURCE_COUNT = 1;
        constexpr std::uint32_t SHADER_PROGRAM_KEY_SHIFT = 32u;
    } // namespace
    platform::RenderResult<types::RenderHandle> OpenGL40Shaders::create_shader_reflection(
        OpenGL40RendererState& runtime,
        ResourceAccessor& resource_accessor,
        RenderHandleMapper& handle_mapper,
        const types::ShaderData& data) {
        auto reflection = std::make_shared<GLSLShaderReflection>();
        auto result = reflection->load(data);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        std::unordered_set<std::string> new_uniform_blocks;
        std::unordered_set<std::string> new_samplers;
        for (const auto& resource :
            reflection->get_shader_input_reflection()->get_input_resources()) {
            if (resource.type == types::ShaderInputResourceType::UniformBuffer &&
                !runtime.uniform_block_bindings.contains(resource.name)) {
                new_uniform_blocks.emplace(resource.name);
            }
            if (resource.type == types::ShaderInputResourceType::Texture &&
                !runtime.sampler_bindings.contains(resource.name)) {
                new_samplers.emplace(resource.name);
            }
        }
        const auto uniform_block_count =
            runtime.uniform_block_bindings.size() + new_uniform_blocks.size();
        if (runtime.uniform_buffer_binding_limit < 0 ||
            uniform_block_count > static_cast<std::size_t>(runtime.uniform_buffer_binding_limit)) {
            return foundation::Error(platform::RenderError::MakeError,
                "OpenGL uniform buffer binding limit is too small for the shader resources");
        }
        const auto sampler_count = runtime.sampler_bindings.size() + new_samplers.size();
        if (runtime.texture_unit_limit < 0 ||
            sampler_count > static_cast<std::size_t>(runtime.texture_unit_limit)) {
            return foundation::Error(platform::RenderError::MakeError,
                "OpenGL texture unit limit is too small for the shader resources");
        }
        for (const auto& resource :
            reflection->get_shader_input_reflection()->get_input_resources()) {
            if (resource.type == types::ShaderInputResourceType::UniformBuffer &&
                !runtime.uniform_block_bindings.contains(resource.name)) {
                runtime.uniform_block_bindings.emplace(resource.name,
                    static_cast<std::uint32_t>(runtime.uniform_block_bindings.size()));
            }
            if (resource.type == types::ShaderInputResourceType::Texture &&
                !runtime.sampler_bindings.contains(resource.name)) {
                std::uint32_t unit = SHADOW_TEXTURE_UNIT;
                if (resource.name != "shadow_texture") {
                    unit = 0;
                    while (unit == SHADOW_TEXTURE_UNIT ||
                           std::ranges::any_of(runtime.sampler_bindings,
                               [unit](const auto& entry) { return entry.second == unit; })) {
                        ++unit;
                    }
                }
                if (!(unit < static_cast<std::uint32_t>(runtime.texture_unit_limit))) {
                    return foundation::Error(platform::RenderError::MakeError,
                        "OpenGL sampler binding exceeds the texture unit limit");
                }
                runtime.sampler_bindings.emplace(resource.name, unit);
            }
        }
        const auto glsl_reflection = reflection;
        auto [resource_handle, _] = resource_accessor.make_shader_reflection(std::move(reflection));
        const auto handle =
            OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::ShaderReflection);
        handle_mapper[handle].resource = resource_handle;
        runtime.reflections.emplace(handle, glsl_reflection);
        return handle;
    }
    platform::RenderResult<types::RenderHandle>
    OpenGL40Shaders::create_vertex_layout_from_shader_data(OpenGL40RendererState& runtime,
        ResourceAccessor& resource_accessor,
        RenderHandleMapper& handle_mapper,
        const types::ShaderData& data) {
        const auto reflection = OpenGL40Shaders::create_shader_reflection(
            runtime, resource_accessor, handle_mapper, data);
        if (reflection.is_err()) {
            return reflection.propagation(platform::RenderError::MakeError);
        }
        return OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::VertexLayout);
    }
    platform::RenderResult<types::RenderHandle> OpenGL40Shaders::create_shader(
        OpenGL40RendererState& runtime,
        RenderHandleMapper& handle_mapper,
        const types::ShaderKind kind,
        const types::ShaderData& data) {
        auto pending = check_gl_errors("before shader creation");
        if (pending.is_err()) {
            return std::move(pending).take_err();
        }
        if (data.binary_type != types::ShaderBinaryType::SourceFileGLSL) {
            return foundation::Error(
                platform::RenderError::MakeError, "OpenGL 4.0 shaders must be GLSL source");
        }
        GLenum stage = NO_GL_OBJECT;
        if (kind == types::ShaderKind::Vertex) {
            stage = GL_VERTEX_SHADER;
        } else if (kind == types::ShaderKind::Pixel) {
            stage = GL_FRAGMENT_SHADER;
        }
        if (stage == NO_GL_OBJECT) {
            return foundation::Error(
                platform::RenderError::MakeError, "Unsupported GLSL shader stage");
        }
        if (data.code.size() > static_cast<std::size_t>((std::numeric_limits<GLint>::max)())) {
            return foundation::Error(
                platform::RenderError::MakeError, "GLSL source length exceeds GLint range");
        }
        const auto shader = glCreateShader(stage);
        if (shader == NO_GL_OBJECT) {
            auto result = check_gl_errors("shader object creation");
            if (result.is_err()) {
                return std::move(result).take_err();
            }
            return foundation::Error(
                platform::RenderError::MakeError, "glCreateShader returned no shader object");
        }
        const auto source = reinterpret_cast<const GLchar*>(data.code.data());
        const auto length = static_cast<GLint>(data.code.size());
        glShaderSource(shader, SHADER_SOURCE_COUNT, &source, &length);
        glCompileShader(shader);
        GLint compiled = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled != GL_TRUE) {
            GLint log_length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
            std::string log;
            if (log_length > 0) {
                log.resize(static_cast<std::size_t>(log_length));
                GLsizei written = 0;
                glGetShaderInfoLog(shader, log_length, &written, log.data());
                log.resize(static_cast<std::size_t>(written));
            }
            auto error = foundation::Error(platform::RenderError::MakeError,
                std::format("GLSL shader compilation failed (stage {:#x}): {}", stage, log));
            auto result = check_gl_errors("shader compilation");
            if (result.is_err()) {
                error.add_message(result.unwrap_err().get_message());
            }
            glDeleteShader(shader);
            return error;
        }
        auto result = check_gl_errors("shader compilation");
        if (result.is_err()) {
            glDeleteShader(shader);
            return std::move(result).take_err();
        }
        const auto handle =
            OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::Shader);
        runtime.shaders.emplace_back(shader);
        runtime.objects.emplace(handle, shader);
        runtime.shader_kinds.emplace(handle, kind);
        return handle;
    }
    bool OpenGL40Shaders::use_active_program(OpenGL40RendererState& runtime) {
        if (runtime.active_vertex_shader == NO_GL_OBJECT ||
            runtime.active_fragment_shader == NO_GL_OBJECT) {
            return false;
        }
        if (runtime.active_program != NO_GL_OBJECT) {
            glUseProgram(runtime.active_program);

            return true;
        }
        const auto program_key =
            (static_cast<std::uint64_t>(runtime.active_vertex_shader) << SHADER_PROGRAM_KEY_SHIFT) |
            runtime.active_fragment_shader;
        if (const auto program = runtime.programs.find(program_key);
            program != runtime.programs.end()) {
            runtime.active_program = program->second;

            glUseProgram(runtime.active_program);
            return true;
        }
        runtime.active_program = glCreateProgram();
        glAttachShader(runtime.active_program, runtime.active_vertex_shader);
        glAttachShader(runtime.active_program, runtime.active_fragment_shader);
        glLinkProgram(runtime.active_program);
        GLint linked = GL_FALSE;
        glGetProgramiv(runtime.active_program, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE) {
            glDeleteProgram(runtime.active_program);

            runtime.active_program = NO_GL_OBJECT;
            return false;
        }
        runtime.programs.emplace(program_key, runtime.active_program);
        glUseProgram(runtime.active_program);
        for (const auto& [name, binding] : runtime.uniform_block_bindings) {
            const auto index = glGetUniformBlockIndex(runtime.active_program, name.c_str());
            if (index != GL_INVALID_INDEX) {
                glUniformBlockBinding(runtime.active_program, index, binding);
            }
        }
        for (const auto& [name, binding] : runtime.sampler_bindings) {
            const auto location = glGetUniformLocation(runtime.active_program, name.c_str());
            if (location != -1) {
                glUniform1i(location, static_cast<GLint>(binding));
            }
        }
        return true;
    }
    void OpenGL40Shaders::submit_command_shader(
        OpenGL40RendererState& runtime, const types::DrawCommand& command) {
        if (command.sub_command != types::SubCommand::Bind) {
            return;
        }
        const auto object = runtime.objects.find(command.handle);
        const auto kind = runtime.shader_kinds.find(command.handle);
        if (object == runtime.objects.end() || kind == runtime.shader_kinds.end()) {
            return;
        }
        if (kind->second == types::ShaderKind::Vertex) {
            runtime.active_vertex_shader = object->second;
        }
        if (kind->second == types::ShaderKind::Pixel) {
            runtime.active_fragment_shader = object->second;
        }
        runtime.active_program = NO_GL_OBJECT;
        OpenGL40Shaders::use_active_program(runtime);
    }
} // namespace enishi::renderer::opengl
