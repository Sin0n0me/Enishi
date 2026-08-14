#pragma once
#include "../../../common/interface_shader_input_reflection.h"
#include "../../../common/interface_shader_reflection.h"
#include <cstdint>
#include <d3d11.h>
#include <d3d11shader.h>
#include <engine_types/assets/shader/shader_data.h>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <optional>
#include <renderer/errors/errors.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    class ShaderReflection : public IShaderReflection<DirectXError>, public IShaderInputReflection {
      private:
        struct InputElementDescription {
            std::string semantic_name;
            ShaderInputInfo info;

            explicit InputElementDescription(std::string&& semantic_name, ShaderInputInfo&& info);
            InputElementDescription(const InputElementDescription& other);
            InputElementDescription(InputElementDescription&& other) noexcept;

            InputElementDescription& operator=(const InputElementDescription& other);
            InputElementDescription& operator=(InputElementDescription&& other) noexcept;

          private:
            void fix_pointer(void);
        };

      private:
        Microsoft::WRL::ComPtr<ID3D11ShaderReflection> reflector;
        std::vector<InputElementDescription> input_element_descriptions;

      public:
        ShaderReflection(void) noexcept = default;
        ShaderReflection(ShaderReflection&&) noexcept = default;
        ShaderReflection& operator=(ShaderReflection&&) noexcept = default;

      public:
        foundation::VoidResult<DirectXError> load(
            const types::ShaderData& shader_data) noexcept override;
        const IShaderInputReflection* get_shader_input_reflection(void) const override;

      public:
        std::uint32_t get_input_count(void) const noexcept override;
        foundation::Option<ShaderInputInfo> get_input(
            const std::uint32_t index) const noexcept override;
        std::vector<ShaderInputInfo> get_inputs(void) const noexcept override;

      public:
        std::vector<D3D11_INPUT_ELEMENT_DESC> get_input_element_descs(void) const noexcept;

      private:
        foundation::Option<std::uint32_t> get(
            const D3D_SHADER_INPUT_TYPE input_type, const std::string& name) const noexcept;

        foundation::VoidResult<DirectXError> load_binding_desc(const std::uint32_t index) noexcept;
        foundation::VoidResult<DirectXError> load_parameter_desc(
            const std::uint32_t index) noexcept;
    };
} // namespace enishi::renderer::directx