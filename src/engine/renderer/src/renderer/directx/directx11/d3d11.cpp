#include "d3d11.h"
#include <foundation/debug/debug.h>

namespace enishi::renderer::directx {
    foundation::VoidResult<DirectXError> D3D11::init(
        const HWND hwnd, const types::WindowSize& size) {
        auto reuslt = this->make_device();
        if (reuslt.is_err()) {
            return reuslt;
        }

        reuslt = this->make_factory();
        if (reuslt.is_err()) {
            return reuslt;
        }

        reuslt = this->make_swap_chain(size);
        if (reuslt.is_err()) {
            return reuslt;
        }

        reuslt = this->make_target(hwnd);
        if (reuslt.is_err()) {
            return reuslt;
        }

        reuslt = this->make_visual();
        if (reuslt.is_err()) {
            return reuslt;
        }

        reuslt = this->commit();
        if (reuslt.is_err()) {
            return reuslt;
        }

        return {};
    }

    foundation::VoidResult<DirectXError> D3D11::make_device(void) {
        const HRESULT result_device = D3D11CreateDevice(nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT |
                (foundation::IS_DEBUG_MODE ? D3D11_CREATE_DEVICE_DEBUG : 0),
            nullptr,
            0,
            D3D11_SDK_VERSION,
            this->device.GetAddressOf(),
            nullptr,
            this->context.GetAddressOf());
        if (FAILED(result_device)) {
            return foundation::Error(
                DirectXError::DeviceError, "D3D11デバイスの作成に失敗しました");
        }

        const HRESULT result_dxgi_device = this->device.As(&this->dxgi_device);
        if (FAILED(result_dxgi_device)) {
            return foundation::Error(DirectXError::DeviceError, "DXGIデバイスの作成に失敗しました");
        }

        const HRESULT result_dcom_device = DCompositionCreateDevice(dxgi_device.Get(),
            __uuidof(this->dcomp_device),
            reinterpret_cast<void**>(this->dcomp_device.GetAddressOf()));
        if (FAILED(result_dcom_device)) {
            return foundation::Error(
                DirectXError::DeviceError, "DCompositionデバイスの作成に失敗しました");
        }

        return {};
    }

    foundation::VoidResult<DirectXError> D3D11::make_factory(void) {
        const HRESULT result_factory = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG,
            __uuidof(this->dxgi_factory),
            reinterpret_cast<void**>(this->dxgi_factory.GetAddressOf()));
        if (FAILED(result_factory)) {
            return foundation::Error(DirectXError::FactoryError, "DXGIFactoryの作成に失敗しました");
        }

        return {};
    }

    foundation::VoidResult<DirectXError> D3D11::make_surface(const types::WindowSize& size) {
        const HRESULT result = this->dcomp_device->CreateSurface(size.width,
            size.height,
            DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_ALPHA_MODE_PREMULTIPLIED,
            this->dcomp_surface.GetAddressOf());
        if (FAILED(result)) {
            return foundation::Error(
                DirectXError::SurfaceError, "Surface(Composition)の作成に失敗しました");
        }

        return {};
    }

    foundation::VoidResult<DirectXError> D3D11::make_swap_chain(const types::WindowSize& size) {
        constexpr DXGI_SAMPLE_DESC sample{.Count = 1};
        const DXGI_SWAP_CHAIN_DESC1 description{
            .Width = static_cast<UINT>(size.width),
            .Height = static_cast<UINT>(size.height),
            .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
            .SampleDesc = sample,
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = 2,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
            .AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED,
        };
        const HRESULT result_swap_chain = this->dxgi_factory->CreateSwapChainForComposition(
            this->dxgi_device.Get(), &description, nullptr, this->dxgi_swap_chain.GetAddressOf());

        if (FAILED(result_swap_chain)) {
            return foundation::Error(
                DirectXError::SwapchainError, "SwapChain(Composition)の作成に失敗しました");
        }

        return {};
    }

    foundation::VoidResult<DirectXError> D3D11::make_target(const HWND hwnd) {
        const HRESULT hr = this->dcomp_device->CreateTargetForHwnd(hwnd,
            true, // Top most
            this->dcomp_target.GetAddressOf());

        if (FAILED(hr)) {
            return foundation::Error(
                DirectXError::TargetError, "Target(Composition)の作成に失敗しました");
        }

        return {};
    }

    foundation::VoidResult<DirectXError> D3D11::make_visual(void) {
        const HRESULT hr = this->dcomp_device->CreateVisual(this->dcomp_visual.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                DirectXError::VisualError, "Visual(Composition)の作成に失敗しました");
        }

        return {};
    }

    foundation::VoidResult<DirectXError> D3D11::commit(void) {
        const HRESULT result_set_context =
            this->dcomp_visual->SetContent(this->dxgi_swap_chain.Get());
        if (FAILED(result_set_context)) {
            return foundation::Error(
                DirectXError::SwapchainError, "SwapChain(Composition)のセットに失敗しました");
        }

        const HRESULT result_set_root = this->dcomp_target->SetRoot(this->dcomp_visual.Get());
        if (FAILED(result_set_root)) {
            return foundation::Error(
                DirectXError::VisualError, "Visual(Composition)のセットに失敗しました");
        }

        // 合成エンジンに完了を通知
        const HRESULT result_commit = this->dcomp_device->Commit();
        if (FAILED(result_commit)) {
            return foundation::Error(DirectXError::DeviceError, "Commitに失敗しました");
        }

        return {};
    }

    foundation::Result<std::unique_ptr<D3D11>, DirectXError> D3D11::make(
        const HWND hwnd, const types::WindowSize& size) {
        std::unique_ptr<D3D11> d3d11 = std::make_unique<D3D11>();

        auto reuslt = d3d11->init(hwnd, size);
        if (reuslt.is_err()) {
            return reuslt.add_message("DirectX11の初期化に失敗しました").error();
        }

        return d3d11;
    }

    Microsoft::WRL::ComPtr<ID3D11Device> D3D11::get_device(void) const {
        return this->device;
    }

    Microsoft::WRL::ComPtr<ID3D11DeviceContext> D3D11::get_context(void) const {
        return this->context;
    }

    Microsoft::WRL::ComPtr<IDXGISwapChain1> D3D11::get_swap_chain(void) const {
        return this->dxgi_swap_chain;
    }

    ID3D11Device* D3D11::get_device_ptr(void) const {
        return this->device.Get();
    }

    ID3D11DeviceContext* D3D11::get_context_ptr(void) const {
        return this->context.Get();
    }

    IDXGISwapChain1* D3D11::get_swap_chain_ptr(void) const {
        return this->dxgi_swap_chain.Get();
    }
} // namespace enishi::renderer::directx