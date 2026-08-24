#pragma once
#include <cstdint>
#include <d3d11.h>
#include <d3d11shader.h>
#include <engine_types/assets/shader/shader_data.h>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <optional>
#include <renderer/common/shader/interface_shader_input_reflection.h>
#include <renderer/common/shader/interface_shader_reflection.h>
#include <renderer/errors/errors.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    class D3D11ShaderReflection : public IShaderReflection, public IShaderInputReflection {
      private:
        types::ShaderKind shader_kind;
        Microsoft::WRL::ComPtr<ID3D11ShaderReflection> reflector;
        std::vector<ShaderInputLayout> input_layouts;
        std::vector<ShaderInputResource> input_resources;
        std::unordered_map<foundation::UTF8, std::size_t> name_to_resource_index;
        std::size_t hash;

      public:
        D3D11ShaderReflection(void) noexcept;
        D3D11ShaderReflection(D3D11ShaderReflection&&) noexcept = default;
        D3D11ShaderReflection& operator=(D3D11ShaderReflection&&) noexcept = default;

      public:
        foundation::VoidResult<RendererError> load(
            const types::ShaderData& shader_data) noexcept override;
        const IShaderInputReflection* get_shader_input_reflection(void) const override;
        types::ShaderKind get_shader_kind(void) const override;

      public:
        std::uint32_t get_input_layout_count(void) const noexcept override;
        foundation::Option<ShaderInputLayout> get_input_layout(
            const std::uint32_t index) const noexcept override;
        std::vector<ShaderInputLayout> get_input_layouts(void) const noexcept override;
        std::uint32_t get_input_resource_count(void) const noexcept override;
        foundation::Option<ShaderInputResource> get_input_resource(
            const std::uint32_t index) const noexcept override;
        std::vector<ShaderInputResource> get_input_resources(void) const noexcept override;
        foundation::Option<ShaderInputResource> resolve_input_resource(
            const foundation::UTF8& name) const noexcept override;

      private:
        foundation::VoidResult<RendererError> load_binding_desc(const std::uint32_t index) noexcept;
        foundation::VoidResult<RendererError> load_parameter_desc(
            const std::uint32_t index) noexcept;
        std::size_t get_shader_hash(void) const override;
    };
} // namespace enishi::renderer::directx