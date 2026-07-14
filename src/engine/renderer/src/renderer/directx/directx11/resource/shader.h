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
    class ShaderPool {
      public:
        using VertexShader = Microsoft::WRL::ComPtr<ID3D11VertexShader>;
        using PixelShader = Microsoft::WRL::ComPtr<ID3D11PixelShader>;
        using ComputeShader = Microsoft::WRL::ComPtr<ID3D11ComputeShader>;
        using HullShader = Microsoft::WRL::ComPtr<ID3D11HullShader>;

      private:
        using Shaders =
            std::variant<std::monostate, VertexShader, PixelShader, ComputeShader, HullShader>;

      private:
        std::unordered_map<types::HandleId, Shaders> handle_to_shader;

      public:
        [[nodiscard]] foundation::VoidResult<DirectXError> create(
            const types::HandleId id, const types::ShaderKind shader_kind) noexcept;

        [[nodiscard]] foundation::Option<types::ShaderKind> get_shader_type(
            const types::HandleId id) const noexcept;

        [[nodiscard]] foundation::Option<VertexShader> get_vertex_shader(
            const types::HandleId id) const noexcept;
        [[nodiscard]] foundation::Option<PixelShader> get_pixel_shader(
            const types::HandleId id) const noexcept;
        [[nodiscard]] foundation::Option<ComputeShader> get_compute_shader(
            const types::HandleId id) const noexcept;
        [[nodiscard]] foundation::Option<HullShader> get_hull_shader(
            const types::HandleId id) const noexcept;

      private:
        [[nodiscard]] foundation::Option<Shaders&> get_shader(const types::HandleId id) noexcept;
        [[nodiscard]] foundation::Option<const Shaders&> get_shader(
            const types::HandleId id) const noexcept;
    };
} // namespace enishi::renderer::directx