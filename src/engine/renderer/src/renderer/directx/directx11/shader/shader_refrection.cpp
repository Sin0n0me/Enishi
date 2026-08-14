#include "shader_refrection.h"
#include <d3dcompiler.h>
#include <foundation/log/logger.h>
#include <renderer/directx/directx11/d3d11_converter.h>

namespace enishi::renderer::directx {
    ShaderReflection::InputElementDescription::InputElementDescription(
        std::string&& semantic_name, ShaderInputInfo&& info)
        : semantic_name(std::move(semantic_name))
        , info(std::move(info)) {
        this->fix_pointer();
    }

    ShaderReflection::InputElementDescription::InputElementDescription(
        const InputElementDescription& other)
        : semantic_name(other.semantic_name)
        , info(other.info) {
        this->fix_pointer();
    }

    ShaderReflection::InputElementDescription::InputElementDescription(
        InputElementDescription&& other) noexcept
        : semantic_name(std::move(other.semantic_name))
        , info(other.info) {
        this->fix_pointer();
    }

    ShaderReflection::InputElementDescription& ShaderReflection::InputElementDescription::operator=(
        const InputElementDescription& other) {
        this->semantic_name = other.semantic_name;
        this->info = other.info;
        this->fix_pointer();
        return *this;
    }

    ShaderReflection::InputElementDescription& ShaderReflection::InputElementDescription::operator=(
        InputElementDescription&& other) noexcept {
        this->semantic_name = std::move(other.semantic_name);
        this->info = other.info;
        this->fix_pointer();
        return *this;
    }

    void ShaderReflection::InputElementDescription::fix_pointer(void) {
        this->info.name = this->semantic_name;
    }

    foundation::VoidResult<DirectXError> ShaderReflection::load(
        const types::ShaderData& shader_data) noexcept {
        {
            const auto hr = D3DReflect(shader_data.code.data(),
                shader_data.code.size(),
                IID_PPV_ARGS(this->reflector.ReleaseAndGetAddressOf()));
            if (FAILED(hr)) {
                return foundation::Error(
                    DirectXError::ShaderReflectionError, "読み込みに失敗しました");
            }
        }

        D3D11_SHADER_DESC shader_desc{};
        {
            const auto hr = this->reflector->GetDesc(&shader_desc);
            if (FAILED(hr)) {
                return foundation::Error(
                    DirectXError::ShaderReflectionError, "Descriptionの取得に失敗しました");
            }
        }

        for (std::uint32_t i = 0; i < shader_desc.InputParameters; ++i) {
            const auto result_parameter_desc = this->load_parameter_desc(i);
            if (result_parameter_desc.is_err()) {
                foundation::Logger::error(result_parameter_desc.unwrap_err().get_message());
            }
        }

        for (std::uint32_t i = 0; i < shader_desc.BoundResources; ++i) {
            const auto result_binding_desc = this->load_binding_desc(i);
            if (result_binding_desc.is_err()) {
                foundation::Logger::error(result_binding_desc.unwrap_err().get_message());
            }
        }

        return {};
    }

    std::vector<D3D11_INPUT_ELEMENT_DESC> ShaderReflection::get_input_element_descs(
        void) const noexcept {
        // 変換
        return this->input_element_descriptions |
               std::views::transform([](const InputElementDescription& desc) {
                   return D3D11Converter::to_input_element_description(desc.info);
               }) |
               std::ranges::to<std::vector>();
    }

    const IShaderInputReflection* ShaderReflection::get_shader_input_reflection(void) const {
        return this;
    }

    foundation::VoidResult<DirectXError> ShaderReflection::load_binding_desc(
        const std::uint32_t index) noexcept {
        D3D11_SHADER_INPUT_BIND_DESC bind_desc{};
        const HRESULT hr = this->reflector->GetResourceBindingDesc(index, &bind_desc);
        if (FAILED(hr)) {
            return foundation::Error(
                DirectXError::ShaderReflectionError, "ResourceBindingDescの取得に失敗しました");
        }

        auto& info = this->input_element_descriptions[index].info;
        info.location = bind_desc.BindPoint;

        return {};
    }

    foundation ::Option<types::VertexFormat> to_vertex_format(
        const D3D11_SIGNATURE_PARAMETER_DESC& desc) noexcept {
        switch (desc.ComponentType) {
            case D3D_REGISTER_COMPONENT_FLOAT32:
                switch (desc.Mask) {
                    case 0x1:
                        return types::VertexFormat::Float32x1;
                    case 0x3:
                        return types::VertexFormat::Float32x2;
                    case 0x7:
                        return types::VertexFormat::Float32x3;
                    case 0xF:
                        return types::VertexFormat::Float32x4;
                    default:
                        break;
                }
                break;
            case D3D_REGISTER_COMPONENT_UINT32:
                switch (desc.Mask) {
                    case 0x1:
                        return types::VertexFormat::UInt32x1;
                    case 0x3:
                        return types::VertexFormat::UInt32x2;
                    case 0x7:
                        return types::VertexFormat::UInt32x3;
                    case 0xF:
                        return types::VertexFormat::UInt32x4;
                    default:
                        break;
                }
                break;
            case D3D_REGISTER_COMPONENT_SINT32:
                switch (desc.Mask) {
                    case 0x1:
                        return types::VertexFormat::Int32x1;
                    case 0x3:
                        return types::VertexFormat::Int32x2;
                    case 0x7:
                        return types::VertexFormat::Int32x3;
                    case 0xF:
                        return types::VertexFormat::Int32x4;
                    default:
                        break;
                }
                break;
            default:
                break;
        }

        return {};
    }

    foundation::VoidResult<DirectXError> ShaderReflection::load_parameter_desc(
        const std::uint32_t index) noexcept {
        D3D11_SIGNATURE_PARAMETER_DESC param_desc;
        const HRESULT hr = this->reflector->GetInputParameterDesc(index, &param_desc);
        if (FAILED(hr)) {
            return foundation::Error(
                DirectXError::ShaderReflectionError, "InputParameterDescの取得に失敗しました");
        }

        // マスクからフォーマットを判定 (R, G, B, A のどれが使われているか)
        const auto opt_format = to_vertex_format(param_desc);
        if (opt_format.is_none()) {
            return foundation::Error(
                DirectXError::ShaderReflectionError, "フォーマットの取得に失敗しました");
        }
        const auto format = opt_format.unwrap();

        // SemanticNameが一時的なものなのでStringで保持
        // 必要であれば取得先で書き換え
        this->input_element_descriptions.emplace_back(
            InputElementDescription(std::string(param_desc.SemanticName),
                ShaderInputInfo{
                    .location = param_desc.SemanticIndex,
                    .array_size = 1,
                    .format = format,
                    .component_count = 0,
                    .component_bit_width = 0,
                    .component_type = ShaderInputComponentType::Double,
                    .offset = D3D11_APPEND_ALIGNED_ELEMENT, // 自動オフセット
                }));

        return {};
    }

    std::uint32_t ShaderReflection::get_input_count(void) const noexcept {
        return this->input_element_descriptions.size();
    }

    foundation::Option<ShaderInputInfo> ShaderReflection::get_input(
        const std::uint32_t index) const noexcept {
        if (this->input_element_descriptions.size() + 1 < index) {
            return {};
        }

        return this->input_element_descriptions.at(index).info;
    }

    std::vector<ShaderInputInfo> ShaderReflection::get_inputs(void) const noexcept {
        return this->input_element_descriptions |
               std::views::transform(
                   [](const InputElementDescription& desc) { return desc.info; }) |
               std::ranges::to<std::vector>();
    }
} // namespace enishi::renderer::directx