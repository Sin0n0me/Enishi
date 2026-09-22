#include "glsl_shader_reflection.h"
#include <algorithm>
#include <cstddef>
#include <regex>

namespace enishi::renderer::opengl {
    constexpr std::size_t INITIAL_SHADER_HASH = 0;
    constexpr std::size_t INPUT_LOCATION_CAPTURE = 1;
    constexpr std::size_t UNIFORM_BINDING_CAPTURE = 1;
    constexpr std::size_t SHADER_TYPE_CAPTURE = 2;
    constexpr std::size_t SHADER_NAME_CAPTURE = 3;
    constexpr std::uint32_t DEFAULT_ARRAY_SIZE = 1;
    constexpr std::uint32_t DEFAULT_COMPONENT = 0;
    constexpr std::uint32_t DEFAULT_COMPONENT_COUNT = 0;
    constexpr std::uint32_t INITIAL_UNIFORM_BINDING = 0;
    constexpr std::uint32_t UNIFORM_BINDING_INCREMENT = 1;
    constexpr std::uint32_t DEFAULT_DESCRIPTOR_SET = 0;
    constexpr std::size_t SHADER_TYPE_PREFIX_OFFSET = 0;

    GLSLShaderReflection::GLSLShaderReflection(void) noexcept
        : kind(types::ShaderKind::Unknown)
        , hash(INITIAL_SHADER_HASH) {
    }

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
            if (resource.name == name) {
                return resource;
            }
        }
        return {};
    }

    foundation::VoidResult<platform::RenderError> GLSLShaderReflection::load(
        const types::ShaderData& shader_data) noexcept {
        if (shader_data.binary_type != types::ShaderBinaryType::SourceFileGLSL) {
            return foundation::Error(
                platform::RenderError::MakeError, "OpenGL 4.0 requires GLSL source");
        }
        this->hash = shader_data.hash();
        const std::string source(
            reinterpret_cast<const char*>(shader_data.code.data()), shader_data.code.size());
        const std::regex input_pattern(
            R"(layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s*in\s+([[:alnum:]_]+)\s+([[:alnum:]_]+))");
        for (std::sregex_iterator it(source.begin(), source.end(), input_pattern), end; it != end;
             ++it) {
            this->input.add_layout({.name = (*it)[SHADER_NAME_CAPTURE].str(),
                .value_type = types::ShaderInputValueType::Float,
                .location =
                    static_cast<std::uint32_t>(std::stoul((*it)[INPUT_LOCATION_CAPTURE].str())),
                .array_size = DEFAULT_ARRAY_SIZE,
                .component = DEFAULT_COMPONENT,
                .component_count = DEFAULT_COMPONENT_COUNT});
        }
        const std::regex uniform_pattern(
            R"((?:layout\s*\(\s*(?:std140\s*,\s*)?(?:binding\s*=\s*(\d+)\s*)?\)\s*)?uniform\s+([[:alnum:]_]+)\s*([[:alnum:]_]*))");
        std::uint32_t next_uniform_binding = INITIAL_UNIFORM_BINDING;
        for (std::sregex_iterator it(source.begin(), source.end(), uniform_pattern), end; it != end;
             ++it) {
            const auto type = (*it)[SHADER_TYPE_CAPTURE].str();
            const auto declared_name = (*it)[SHADER_NAME_CAPTURE].str();
            const auto name = declared_name.empty() ? type : declared_name;
            const auto binding =
                (*it)[UNIFORM_BINDING_CAPTURE].matched
                    ? static_cast<std::uint32_t>(std::stoul((*it)[UNIFORM_BINDING_CAPTURE].str()))
                    : next_uniform_binding++;
            next_uniform_binding =
                std::max(next_uniform_binding, binding + UNIFORM_BINDING_INCREMENT);
            this->input.add_resource({.name = name,
                .type = type.find("sampler") == SHADER_TYPE_PREFIX_OFFSET
                            ? types::ShaderInputResourceType::Texture
                            : types::ShaderInputResourceType::UniformBuffer,
                .dimension = types::ShaderInputResourceDimension::Texture2D,
                .read_only = types::ShaderInputResourceAccess::ReadOnly,
                .set = DEFAULT_DESCRIPTOR_SET,
                .binding = binding,
                .array_size = DEFAULT_ARRAY_SIZE});
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
