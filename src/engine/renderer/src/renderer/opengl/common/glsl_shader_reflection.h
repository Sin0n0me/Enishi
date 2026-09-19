#pragma once

#include <platform/renderer/shader/interface_shader_reflection.h>

namespace enishi::renderer::opengl {
    class GLSLShaderInputReflection final : public platform::IShaderInputReflection {
      private:
        std::vector<types::ShaderInputLayout> layouts;
        std::vector<types::ShaderInputResource> resources;

      public:
        void add_layout(types::ShaderInputLayout layout);
        void add_resource(types::ShaderInputResource resource);
        std::uint32_t get_input_layout_count(void) const noexcept override;
        foundation::Option<types::ShaderInputLayout> get_input_layout(std::uint32_t index) const noexcept override;
        std::vector<types::ShaderInputLayout> get_input_layouts(void) const noexcept override;
        std::uint32_t get_input_resource_count(void) const noexcept override;
        foundation::Option<types::ShaderInputResource> get_input_resource(std::uint32_t index) const noexcept override;
        std::vector<types::ShaderInputResource> get_input_resources(void) const noexcept override;
        foundation::Option<types::ShaderInputResource> resolve_input_resource(const foundation::UTF8& name) const noexcept override;
    };

    class GLSLShaderReflection final : public platform::IShaderReflection {
      private:
        GLSLShaderInputReflection input;
        types::ShaderKind kind = types::ShaderKind::Unknown;
        std::size_t hash = 0;

      public:
        foundation::VoidResult<platform::RenderError> load(const types::ShaderData& shader_data) noexcept override;
        const platform::IShaderInputReflection* get_shader_input_reflection(void) const override;
        types::ShaderKind get_shader_kind(void) const override;
        std::size_t get_shader_hash(void) const override;
    };
} // namespace enishi::renderer::opengl
