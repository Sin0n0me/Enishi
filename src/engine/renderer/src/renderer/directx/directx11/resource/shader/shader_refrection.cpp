#include "shader_refrection.h"
#include <d3dcompiler.h>
#include <foundation/log/logger.h>
#include <renderer/directx/directx11/d3d11_converter.h>

namespace enishi::renderer::directx {
    ShaderReflection::ShaderReflection(void) noexcept
        : shader_kind(types::ShaderKind::Unknown) {
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

        this->input_layouts.resize(shader_desc.InputParameters);
        for (std::uint32_t i = 0; i < shader_desc.InputParameters; ++i) {
            const auto result_parameter_desc = this->load_parameter_desc(i);
            if (result_parameter_desc.is_err()) {
                foundation::Logger::error(result_parameter_desc.unwrap_err().get_message());
            }
        }

        this->input_resources.resize(shader_desc.BoundResources);
        for (std::uint32_t i = 0; i < shader_desc.BoundResources; ++i) {
            const auto result_binding_desc = this->load_binding_desc(i);
            if (result_binding_desc.is_err()) {
                foundation::Logger::error(result_binding_desc.unwrap_err().get_message());
            }
        }

        const D3D11_SHADER_VERSION_TYPE shaderType =
            static_cast<D3D11_SHADER_VERSION_TYPE>(D3D11_SHVER_GET_TYPE(shader_desc.Version));

        this->shader_kind = [&]() {
            switch (shaderType) {
                case D3D11_SHVER_VERTEX_SHADER:
                    return types::ShaderKind::Vertex;
                case D3D11_SHVER_PIXEL_SHADER:
                    return types::ShaderKind::Pixel;
                case D3D11_SHVER_HULL_SHADER:
                    return types::ShaderKind::Hull;
                case D3D11_SHVER_DOMAIN_SHADER:
                    return types::ShaderKind::Domain;
                case D3D11_SHVER_GEOMETRY_SHADER:
                    return types::ShaderKind::Geometry;
                case D3D11_SHVER_COMPUTE_SHADER:
                    return types::ShaderKind::Compute;
                default:
                    break;
            }
            return types::ShaderKind::Unknown;
        }();

        return {};
    }

    const IShaderInputReflection* ShaderReflection::get_shader_input_reflection(void) const {
        return this;
    }

    types::ShaderKind ShaderReflection::get_shader_kind(void) const {
        return this->shader_kind;
    }

    foundation::VoidResult<DirectXError> ShaderReflection::load_binding_desc(
        const std::uint32_t index) noexcept {
        D3D11_SHADER_INPUT_BIND_DESC bind_desc{};
        const HRESULT hr = this->reflector->GetResourceBindingDesc(index, &bind_desc);
        if (FAILED(hr)) {
            return foundation::Error(
                DirectXError::ShaderReflectionError, "ResourceBindingDescの取得に失敗しました");
        }

        const auto type = [&bind_desc]() {
            switch (bind_desc.Type) {
                case D3D_SIT_CBUFFER:
                case D3D_SIT_TBUFFER:
                    return ShaderInputResourceType::UniformBuffer;
                case D3D_SIT_STRUCTURED:
                case D3D_SIT_BYTEADDRESS:
                    return ShaderInputResourceType::StorageBuffer;
                case D3D_SIT_TEXTURE:
                    return ShaderInputResourceType::Texture;
                case D3D_SIT_SAMPLER:
                    return ShaderInputResourceType::Sampler;
                case D3D_SIT_UAV_RWTYPED:
                case D3D_SIT_UAV_RWSTRUCTURED:
                case D3D_SIT_UAV_RWBYTEADDRESS:
                case D3D_SIT_UAV_APPEND_STRUCTURED:
                case D3D_SIT_UAV_CONSUME_STRUCTURED:
                case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                    return ShaderInputResourceType::StorageTexture;
                default:
                    break;
            }
            return ShaderInputResourceType::Unknown;
        }();
        const auto dimension = [&bind_desc]() {
            switch (bind_desc.Dimension) {
                case D3D_SRV_DIMENSION_BUFFER:
                    return ShaderInputResourceDimension::Buffer;
                case D3D_SRV_DIMENSION_TEXTURE1D:
                    return ShaderInputResourceDimension::Texture1D;
                case D3D_SRV_DIMENSION_TEXTURE2D:
                    return ShaderInputResourceDimension::Texture2D;
                case D3D_SRV_DIMENSION_TEXTURE3D:
                    return ShaderInputResourceDimension::Texture3D;
                case D3D_SRV_DIMENSION_TEXTURECUBE:
                    return ShaderInputResourceDimension::TextureCube;
                case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
                    return ShaderInputResourceDimension::Texture1DArray;
                case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
                    return ShaderInputResourceDimension::Texture2DArray;
                case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
                    return ShaderInputResourceDimension::TextureCubeArray;
                case D3D_SRV_DIMENSION_TEXTURE2DMS:
                    return ShaderInputResourceDimension::Texture2DMS;
                case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
                    return ShaderInputResourceDimension::Texture2DMSArray;
                default:
                    break;
            }

            return ShaderInputResourceDimension::Unknown;
        }();

        this->input_resources[index] = ShaderInputResource{
            .name = bind_desc.Name,
            .type = type,
            .dimension = dimension,
            .set = 0, // DirectX11に対応するものはない
            .binding = bind_desc.BindPoint,
            .array_size = bind_desc.BindCount,
        };

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
        const auto value_type = [&param_desc]() {
            switch (param_desc.ComponentType) {
                case D3D_REGISTER_COMPONENT_FLOAT32:
                    return ShaderInputValueType::Float;
                case D3D_REGISTER_COMPONENT_UINT32:
                    return ShaderInputValueType::UnsignedInteger;
                case D3D_REGISTER_COMPONENT_SINT32:
                    return ShaderInputValueType::SignedInteger;
                default:
                    break;
            }
            return ShaderInputValueType::Unknown;
        }();
        const auto component_count = [&param_desc]() -> std::uint32_t {
            switch (param_desc.Mask) {
                case 0b0001:
                    return 1;
                case 0b0011:
                    return 2;
                case 0b0111:
                    return 3;
                case 0b1111:
                    return 4;
                default:
                    break;
            }
            return 0;
        }();

        // SemanticNameが一時的なものなのでStringで保持
        this->input_layouts[index] = ShaderInputLayout{
            .name = param_desc.SemanticName,
            .value_type = value_type,
            .location = param_desc.SemanticIndex,
            .array_size = 1,
            .component = 0, // DirextXでは関係ない
            .component_count = component_count,
        };

        return {};
    }

    std::uint32_t ShaderReflection::get_input_layout_count(void) const noexcept {
        return this->input_layouts.size();
    }

    foundation::Option<ShaderInputLayout> ShaderReflection::get_input_layout(
        const std::uint32_t index) const noexcept {
        if (this->input_layouts.size() + 1 < index) {
            return {};
        }

        return this->input_layouts.at(index);
    }

    std::vector<ShaderInputLayout> ShaderReflection::get_input_layouts(void) const noexcept {
        return this->input_layouts;
    }

    std::uint32_t ShaderReflection::get_input_resource_count(void) const noexcept {
        return this->input_resources.size();
    }

    foundation::Option<ShaderInputResource> ShaderReflection::get_input_resource(
        const std::uint32_t index) const noexcept {
        if (this->input_resources.size() + 1 < index) {
            return {};
        }

        return this->input_resources.at(index);
    }

    std::vector<ShaderInputResource> ShaderReflection::get_input_resources(void) const noexcept {
        return this->input_resources;
    }
} // namespace enishi::renderer::directx