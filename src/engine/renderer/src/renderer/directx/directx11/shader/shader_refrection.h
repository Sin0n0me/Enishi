#pragma once
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
    struct InputElementDescription {
        std::string semantic_name;
        D3D11_INPUT_ELEMENT_DESC description;

        explicit InputElementDescription(
            std::string&& semantic_name, const D3D11_INPUT_ELEMENT_DESC description);
        InputElementDescription(const InputElementDescription& other);
        InputElementDescription(InputElementDescription&& other) noexcept;

        InputElementDescription& operator=(const InputElementDescription& other);
        InputElementDescription& operator=(InputElementDescription&& other) noexcept;

      private:
        void fix_pointer(void);
    };

    class ShaderReflection {
      private:
        Microsoft::WRL::ComPtr<ID3D11ShaderReflection> reflector;
        std::unordered_map<D3D_SHADER_INPUT_TYPE, std::unordered_map<std::string, std::uint32_t>>
            binding_slot_map;
        std::vector<InputElementDescription> input_element_descriptions;

      public:
        static foundation::Result<ShaderReflection, DirectXError> make(
            const types::ShaderData& data);

        ShaderReflection(ShaderReflection&&) noexcept = default;
        ShaderReflection& operator=(ShaderReflection&&) noexcept = default;

        foundation::Option<std::uint32_t> get_constant_buffer_slot(
            const std::string& name) const noexcept;

        foundation::Option<std::uint32_t> get_sampler_slot(const std::string& name) const noexcept;

        const std::vector<InputElementDescription>& get_input_element_descs(void) const noexcept;

      private:
        ShaderReflection(void) noexcept = default;

        foundation::Option<std::uint32_t> get(
            const D3D_SHADER_INPUT_TYPE input_type, const std::string& name) const noexcept;

        foundation::VoidResult<DirectXError> load(void) noexcept;
        foundation::VoidResult<DirectXError> load_binding_desc(const std::uint32_t index) noexcept;
        foundation::VoidResult<DirectXError> load_parameter_desc(
            const std::uint32_t index) noexcept;
    };
} // namespace enishi::renderer::directx