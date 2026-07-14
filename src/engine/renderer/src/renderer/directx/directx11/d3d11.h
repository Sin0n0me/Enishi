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
        foundation::VoidResult<DirectXError> init(const HWND hwnd, const types::WindowSize& size);
        foundation::VoidResult<DirectXError> make_device(void);
        foundation::VoidResult<DirectXError> make_factory(void);
        foundation::VoidResult<DirectXError> make_surface(const types::WindowSize& size);
        foundation::VoidResult<DirectXError> make_swap_chain(const types::WindowSize& size);
        foundation::VoidResult<DirectXError> make_target(const HWND hwnd);
        foundation::VoidResult<DirectXError> make_visual(void);
        foundation::VoidResult<DirectXError> commit(void);

      public:
        static foundation::Result<std::unique_ptr<D3D11>, DirectXError> make(
            const HWND hwnd, const types::WindowSize& size);

        Microsoft::WRL::ComPtr<ID3D11Device> get_device(void) const override;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> get_context(void) const override;
        Microsoft::WRL::ComPtr<IDXGISwapChain1> get_swap_chain(void) const override;
        ID3D11Device* get_device_ptr(void) const override;
        ID3D11DeviceContext* get_context_ptr(void) const override;
        IDXGISwapChain1* get_swap_chain_ptr(void) const override;
    };
} // namespace enishi::renderer::directx