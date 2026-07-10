#pragma once
#include "../../../errors/errors.h"
#include <d3d11.h>
#include <engine_types/assets/shader/shader_kind.h>
#include <engine_types/handle/handle_type.h>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    namespace {
        struct ShaderInfo {
            types::ShaderKind shader_type;
            std::uint32_t index;
        };
    } // namespace

    class ShaderPool {
      private:
        using Shaders = std::variant<std::monostate,
            Microsoft::WRL::ComPtr<ID3D11VertexShader>,
            Microsoft::WRL::ComPtr<ID3D11PixelShader>,
            Microsoft::WRL::ComPtr<ID3D11ComputeShader>>;

        std::unordered_map<types::HandleId, ShaderInfo> handle_map;
        std::vector<Microsoft::WRL::ComPtr<ID3D11VertexShader>> vertex_shaders;
        std::vector<Microsoft::WRL::ComPtr<ID3D11PixelShader>> pixel_shaders;
        std::vector<Microsoft::WRL::ComPtr<ID3D11ComputeShader>> compute_shaders;

      private:
        [[nodiscard]] foundation::Option<const ShaderInfo&> get_shader_info(
            const types::HandleId id) const noexcept;

      public:
        [[nodiscard]] foundation::VoidResult<DirectXError> create(
            const types::HandleId id, const types::ShaderKind shader_kind) noexcept;

        [[nodiscard]] foundation::Option<types::ShaderKind> get_shader_type(
            const types::HandleId id) const noexcept;

        [[nodiscard]] foundation::Option<ID3D11VertexShader*> get_vertex_shader(
            const types::HandleId id) const noexcept;
        [[nodiscard]] foundation::Option<ID3D11PixelShader*> get_pixel_shader(
            const types::HandleId id) const noexcept;
        [[nodiscard]] foundation::Option<ID3D11ComputeShader*> get_compute_shader(
            const types::HandleId id) const noexcept;
        [[nodiscard]] foundation::Option<ID3D11VertexShader* const*> get_address_vertex_shader(
            const types::HandleId id) const noexcept;
        [[nodiscard]] foundation::Option<ID3D11PixelShader* const*> get_address_pixel_shader(
            const types::HandleId id) const noexcept;
        [[nodiscard]] foundation::Option<ID3D11ComputeShader* const*> get_address_compute_shader(
            const types::HandleId id) const noexcept;
        [[nodiscard]] foundation::Option<ID3D11VertexShader**> get_address_vertex_shader(
            const types::HandleId id) noexcept;
        [[nodiscard]] foundation::Option<ID3D11PixelShader**> get_address_pixel_shader(
            const types::HandleId id) noexcept;
        [[nodiscard]] foundation::Option<ID3D11ComputeShader**> get_address_compute_shader(
            const types::HandleId id) noexcept;
    };
} // namespace enishi::renderer::directx