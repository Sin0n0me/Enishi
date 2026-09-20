#include "glsl_shader_reflection.h"
#include <regex>

namespace enishi::renderer::opengl {
    void GLSLShaderInputReflection::add_layout(types::ShaderInputLayout layout) {
        this->layouts.emplace_back(std::move(layout));
    }
    void GLSLShaderInputReflection::add_resource(types::ShaderInputResource resource) {
        this->resources.emplace_back(std::move(resource));
    }
    std::uint32_t GLSLShaderInputReflection::get_input_layout_count(void) const noexcept {
        return static_cast<std::uint32_t>(this->layouts.size());
    }
    foundation::Option<types::ShaderInputLayout> GLSLShaderInputReflection::get_input_layout(
        const std::uint32_t index) const noexcept {
        return index < this->layouts.size()
                   ? foundation::Option<types::ShaderInputLayout>(this->layouts[index])
                   : foundation::Option<types::ShaderInputLayout>();
    }
    std::vector<types::ShaderInputLayout> GLSLShaderInputReflection::get_input_layouts(
        void) const noexcept {
        return this->layouts;
    }
    std::uint32_t GLSLShaderInputReflection::get_input_resource_count(void) const noexcept {
        return static_cast<std::uint32_t>(this->resources.size());
    }
    foundation::Option<types::ShaderInputResource> GLSLShaderInputReflection::get_input_resource(
        const std::uint32_t index) const noexcept {
        return index < this->resources.size()
                   ? foundation::Option<types::ShaderInputResource>(this->resources[index])
                   : foundation::Option<types::ShaderInputResource>();
    }
    std::vector<types::ShaderInputResource> GLSLShaderInputReflection::get_input_resources(
        void) const noexcept {
        return this->resources;
    }
    foundation::Option<types::ShaderInputResource>
    GLSLShaderInputReflection::resolve_input_resource(const foundation::UTF8& name) const noexcept {
        for (const auto& resource : this->resources) {
            if (resource.name == name)
                return resource;
        }
        return {};
    }

    foundation::VoidResult<platform::RenderError> GLSLShaderReflection::load(
        const types::ShaderData& shader_data) noexcept {
        if (shader_data.binary_type != types::ShaderBinaryType::SourceFileGLSL)
            return foundation::Error(
                platform::RenderError::MakeError, "OpenGL 4.0 requires GLSL source");
        this->hash = shader_data.hash();
        const std::string source(
            reinterpret_cast<const char*>(shader_data.code.data()), shader_data.code.size());
        const std::regex input_pattern(
            R"(layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s*in\s+([[:alnum:]_]+)\s+([[:alnum:]_]+))");
        for (std::sregex_iterator it(source.begin(), source.end(), input_pattern), end; it != end;
            ++it) {
            this->input.add_layout({.name = (*it)[3].str(),
                .value_type = types::ShaderInputValueType::Float,
                .location = static_cast<std::uint32_t>(std::stoul((*it)[1].str())),
                .array_size = 1,
                .component = 0,
                .component_count = 0});
        }
        const std::regex uniform_pattern(
            R"((?:layout\s*\(\s*(?:std140\s*,\s*)?(?:binding\s*=\s*(\d+)\s*)?\)\s*)?uniform\s+([[:alnum:]_]+)\s*([[:alnum:]_]*))");
        std::uint32_t next_uniform_binding = 0;
        for (std::sregex_iterator it(source.begin(), source.end(), uniform_pattern), end; it != end;
            ++it) {
            const auto type = (*it)[2].str();
            const auto declared_name = (*it)[3].str();
            const auto name = declared_name.empty() ? type : declared_name;
            const auto binding = (*it)[1].matched
                                     ? static_cast<std::uint32_t>(std::stoul((*it)[1].str()))
                                     : next_uniform_binding++;
            this->input.add_resource({.name = name,
                .type = type.find("sampler") == 0 ? types::ShaderInputResourceType::Texture
                                                  : types::ShaderInputResourceType::UniformBuffer,
                .dimension = types::ShaderInputResourceDimension::Texture2D,
                .read_only = types::ShaderInputResourceAccess::ReadOnly,
                .set = 0,
                .binding = binding,
                .array_size = 1});
        }
        return {};
    }
    const platform::IShaderInputReflection* GLSLShaderReflection::get_shader_input_reflection(
        void) const {
        return &this->input;
    }
    types::ShaderKind GLSLShaderReflection::get_shader_kind(void) const {
        return this->kind;
    }
    std::size_t GLSLShaderReflection::get_shader_hash(void) const {
        return this->hash;
    }
} // namespace enishi::renderer::opengl