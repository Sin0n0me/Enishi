#pragma once
#include <d3d11.h>
#include <foundation/option/option.h>
#include <tuple>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    class IViewAccessor {
      public:
        using DepthStencilView = Microsoft::WRL::ComPtr<ID3D11DepthStencilView>;
        using RenderTargetView = Microsoft::WRL::ComPtr<ID3D11RenderTargetView>;
        using ShaderResourceView = Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>;
        using UnorderedAccessView = Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>;

      public:
        virtual ~IViewAccessor(void) noexcept = default;
    };
} // namespace enishi::renderer::directx