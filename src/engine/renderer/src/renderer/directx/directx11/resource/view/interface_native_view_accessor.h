#pragma once
#include <d3d11.h>
#include <engine_types/handle/handle_type.h>
#include <foundation/option/option.h>
#include <tuple>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    class INativeViewAccessor {
      public:
        using NativeDepthStencilView = Microsoft::WRL::ComPtr<ID3D11DepthStencilView>;
        using NativeRenderTargetView = Microsoft::WRL::ComPtr<ID3D11RenderTargetView>;
        using NativeShaderResourceView = Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>;
        using NativeUnorderedAccessView = Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>;

      public:
        virtual ~INativeViewAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<types::HandleId, NativeDepthStencilView&>
        make_native_depth_stencil_view(void) noexcept = 0;
        [[nodiscard]] virtual std::tuple<types::HandleId, NativeRenderTargetView&>
        make_native_render_target_view(void) noexcept = 0;
        [[nodiscard]] virtual std::tuple<types::HandleId, NativeShaderResourceView&>
        make_native_shader_resource_view(void) noexcept = 0;
        [[nodiscard]] virtual std::tuple<types::HandleId, NativeUnorderedAccessView&>
        make_native_unordered_access_view(void) noexcept = 0;
        [[nodiscard]] virtual void remove_native_view(const types::HandleId handle) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<const NativeDepthStencilView&>
        get_native_depth_stencil_view(const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<NativeDepthStencilView&>
        get_native_depth_stencil_view(const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeRenderTargetView&>
        get_native_render_target_view(const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<NativeRenderTargetView&>
        get_native_render_target_view(const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeShaderResourceView&>
        get_native_shader_resource_view(const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<NativeShaderResourceView&>
        get_native_shader_resource_view(const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeUnorderedAccessView&>
        get_native_unordered_access_view(const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<NativeUnorderedAccessView&>
        get_native_unordered_access_view(const types::HandleId handle) noexcept = 0;
    };
} // namespace enishi::renderer::directx