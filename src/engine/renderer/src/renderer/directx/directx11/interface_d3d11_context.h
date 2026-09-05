#pragma once
#include <d3d11.h>
#include <dcomp.h>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    class ID3D11Context {
      public:
        using Device = Microsoft::WRL::ComPtr<ID3D11Device>;
        using Context = Microsoft::WRL::ComPtr<ID3D11DeviceContext>;
        using SwapChain = Microsoft::WRL::ComPtr<IDXGISwapChain1>;

      public:
        virtual ~ID3D11Context(void) noexcept = default;

        [[nodiscard]] virtual Device get_device(void) const = 0;
        [[nodiscard]] virtual Context get_context(void) const = 0;
        [[nodiscard]] virtual SwapChain get_swap_chain(void) const = 0;
    };
} // namespace enishi::renderer::directx