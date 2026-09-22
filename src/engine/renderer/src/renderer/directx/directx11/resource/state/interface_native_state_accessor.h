#pragma once
#include <d3d11.h>
#include <engine_types/handle/handle_type.h>
#include <foundation/option/option.h>
#include <tuple>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    class INativeStateAccessor {
      public:
        using NativeRasterizerState = Microsoft::WRL::ComPtr<ID3D11RasterizerState>;
        using NativeBlendState = Microsoft::WRL::ComPtr<ID3D11BlendState>;
        using NativeDepthStencilState = Microsoft::WRL::ComPtr<ID3D11DepthStencilState>;
        using NativeSamplerState = Microsoft::WRL::ComPtr<ID3D11SamplerState>;

      public:
        virtual ~INativeStateAccessor(void) noexcept = default;

        [[nodiscard]] virtual void remove_native_state(const types::HandleId handle) noexcept = 0;

        [[nodiscard]] virtual std::tuple<types::HandleId, NativeRasterizerState&>
        make_native_rasterizer_state(void) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<NativeRasterizerState&>
        get_native_rasterizer_state(const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeRasterizerState&>
        get_native_rasterizer_state(const types::HandleId handle) const noexcept = 0;

        [[nodiscard]] virtual std::tuple<types::HandleId, NativeBlendState&>
        make_native_blend_state(void) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<NativeBlendState&> get_native_blend_state(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeBlendState&> get_native_blend_state(
            const types::HandleId handle) const noexcept = 0;

        [[nodiscard]] virtual std::tuple<types::HandleId, NativeDepthStencilState&>
        make_native_depth_stencil_state(void) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<NativeDepthStencilState&>
        get_native_depth_stencil_state(const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeDepthStencilState&>
        get_native_depth_stencil_state(const types::HandleId handle) const noexcept = 0;

        [[nodiscard]] virtual std::tuple<types::HandleId, NativeSamplerState&>
        make_native_sampler_state(void) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<NativeSamplerState&> get_native_sampler_state(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeSamplerState&>
        get_native_sampler_state(const types::HandleId handle) const noexcept = 0;
    };
} // namespace enishi::renderer::directx