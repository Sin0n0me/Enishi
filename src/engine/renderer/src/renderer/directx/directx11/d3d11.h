#pragma once
#include "../../errors/errors.h"
#include "interface_d3d11_context.h"
#include <d3d11.h>
#include <dcomp.h>
#include <engine_types/window/window_types.h>
#include <foundation/result/result.h>
#include <memory>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    class D3D11 : public ID3D11Context {
      private:
        Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_device;
        Microsoft::WRL::ComPtr<IDXGIAdapter> dxgi_adapter;
        Microsoft::WRL::ComPtr<IDXGIFactory2> dxgi_factory;
        Microsoft::WRL::ComPtr<IDXGISwapChain1> dxgi_swap_chain;
        Microsoft::WRL::ComPtr<IDCompositionDevice> dcomp_device;
        Microsoft::WRL::ComPtr<IDCompositionTarget> dcomp_target;
        Microsoft::WRL::ComPtr<IDCompositionVisual> dcomp_visual;
        Microsoft::WRL::ComPtr<IDCompositionSurface> dcomp_surface;
        Microsoft::WRL::ComPtr<ID3D11Device> device;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;

      private:
        foundation::VoidResult<RendererError> init(const HWND hwnd, const types::WindowSize& size);
        foundation::VoidResult<RendererError> make_device(void);
        foundation::VoidResult<RendererError> make_factory(void);
        foundation::VoidResult<RendererError> make_surface(const types::WindowSize& size);
        foundation::VoidResult<RendererError> make_swap_chain(const types::WindowSize& size);
        foundation::VoidResult<RendererError> make_target(const HWND hwnd);
        foundation::VoidResult<RendererError> make_visual(void);
        foundation::VoidResult<RendererError> commit(void);

      public:
        static foundation::Result<std::unique_ptr<D3D11>, RendererError> make(
            const HWND hwnd, const types::WindowSize& size);

        D3D11::Device get_device(void) const override;
        D3D11::Context get_context(void) const override;
        D3D11::SwapChain get_swap_chain(void) const override;
    };
} // namespace enishi::renderer::directx