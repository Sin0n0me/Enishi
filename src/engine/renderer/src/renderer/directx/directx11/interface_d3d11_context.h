#pragma once
#include <d3d11.h>
#include <dcomp.h>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    class ID3D11Context {
      public:
        virtual ~ID3D11Context(void) noexcept = default;

        [[nodiscard]] virtual Microsoft::WRL::ComPtr<ID3D11Device> get_device(void) const = 0;
        [[nodiscard]] virtual Microsoft::WRL::ComPtr<ID3D11DeviceContext> get_context(
            void) const = 0;
        [[nodiscard]] virtual Microsoft::WRL::ComPtr<IDXGISwapChain1> get_swap_chain(
            void) const = 0;
        [[nodiscard]] virtual ID3D11Device* get_device_ptr(void) const = 0;
        [[nodiscard]] virtual ID3D11DeviceContext* get_context_ptr(void) const = 0;
        [[nodiscard]] virtual IDXGISwapChain1* get_swap_chain_ptr(void) const = 0;
    };
} // namespace enishi::renderer::directx