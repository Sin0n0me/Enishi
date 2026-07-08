#include "shader_refrection.h"
#include <d3dcompiler.h>

namespace enishi::renderer::directx {
    InputElementDescription::InputElementDescription(
        std::string&& semantic_name, const D3D11_INPUT_ELEMENT_DESC description)
        : semantic_name(std::move(semantic_name))
        , description(description) {
        this->description.SemanticName = this->semantic_name.c_str();
    }

    foundation::Result<ShaderReflection, DirectXError> ShaderReflection::make(
        const types::ShaderData& data) {
        ShaderReflection reflection;

        const HRESULT hr = D3DReflect(
            data.code.data(), data.code.size(), IID_PPV_ARGS(reflection.reflector.GetAddressOf()));
        if (FAILED(hr)) {
            return foundation::Error(DirectXError::ShaderReflectionError);
        }

        return reflection;
    }

    foundation::Option<std::uint32_t> ShaderReflection::get_constant_buffer_slot(
        const std::string& name) const noexcept {
        return this->get(D3D_SHADER_INPUT_TYPE::D3D10_SIT_CBUFFER, name);
    }

    foundation::Option<std::uint32_t> ShaderReflection::get_sampler_slot(
        const std::string& name) const noexcept {
        return this->get(D3D_SHADER_INPUT_TYPE::D3D_SIT_SAMPLER, name);
    }

    const std::vector<InputElementDescription>& ShaderReflection::get_input_element_descs(
        void) const noexcept {
        return this->input_element_descriptions;
    }

    foundation::Option<std::uint32_t> ShaderReflection::get(
        const D3D_SHADER_INPUT_TYPE input_type, const std::string& name) const noexcept {
        const auto& type_iter =
            this->binding_slot_map.find(D3D_SHADER_INPUT_TYPE::D3D10_SIT_CBUFFER);
        if (type_iter == this->binding_slot_map.end()) {
            return {};
        }

        const auto& name_iter = type_iter->second;
        const auto& iter2 = name_iter.find(name);
        if (iter2 == name_iter.end()) {
            return {};
        }

        return iter2->second;
    }

    foundation::VoidResult<DirectXError> ShaderReflection::load(void) noexcept {
        D3D11_SHADER_DESC shader_desc{};
        const HRESULT hr = this->reflector->GetDesc(&shader_desc);
        if (FAILED(hr)) {
            return foundation::Error(DirectXError::ShaderReflectionError);
        }

        for (std::uint32_t i = 0; i < shader_desc.BoundResources; ++i) {
            this->load_binding_desc(i);
            this->load_parameter_desc(i);
        }

        return {};
    }

    foundation::VoidResult<DirectXError> ShaderReflection::load_binding_desc(
        const std::uint32_t index) noexcept {
        D3D11_SHADER_INPUT_BIND_DESC bind_desc{};
        const HRESULT hr = this->reflector->GetResourceBindingDesc(index, &bind_desc);
        if (FAILED(hr)) {
            return {};
        }

        this->binding_slot_map[bind_desc.Type][bind_desc.Name] = bind_desc.BindPoint;

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
        DXGI_FORMAT fromat = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
        if (param_desc.Mask == 0b0001) {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) {
                fromat = DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
            } else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) {
                fromat = DXGI_FORMAT::DXGI_FORMAT_R32_SINT;
            } else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) {
                fromat = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
            }
        } else if (param_desc.Mask <= 0b0011) {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) {
                fromat = DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT;
            } else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) {
                fromat = DXGI_FORMAT::DXGI_FORMAT_R32G32_SINT;
            } else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) {
                fromat = DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
            }
        } else if (param_desc.Mask <= 0b0111) {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) {
                fromat = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_UINT;
            } else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) {
                fromat = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_SINT;
            } else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) {
                fromat = DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
            }
        } else if (param_desc.Mask <= 0b1111) {
            if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) {
                fromat = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_UINT;
            } else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) {
                fromat = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT;
            } else if (param_desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) {
                fromat = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
        }

        // SemanticNameが一時的なものなのでStringで保持
        this->input_element_descriptions.emplace_back(
            InputElementDescription(std::string(param_desc.SemanticName),
                D3D11_INPUT_ELEMENT_DESC{
                    .SemanticIndex = param_desc.SemanticIndex,
                    .Format = fromat,
                    .AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT, // 自動オフセット
                    .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
                    .InstanceDataStepRate = 0,
                }));

        return {};
    }
} // namespace enishi::renderer::directx