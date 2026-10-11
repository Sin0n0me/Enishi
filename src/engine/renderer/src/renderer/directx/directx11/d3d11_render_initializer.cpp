#include "d3d11_render_initializer.h"

namespace enishi::renderer::directx {
    D3D11RenderInitializer::D3D11RenderInitializer(const bool use_dcomp) noexcept
        : use_dcomp(use_dcomp) {
    }

    foundation::Result<std::shared_ptr<D3D11Renderer>, platform::RenderError>
    D3D11RenderInitializer::init(
        const platform::WindowHandle& window_handle, const types::WindowSize& window_size) {
        if (window_handle.tag != platform::WindowSystem::Windows) {
            return foundation::Error(
                platform::RenderError::MakeError, "Windows以外では動作しません");
        }

        const auto native_handle = window_handle.native_handle.windows;

        std::unique_ptr<D3D11> d3d11;

        // RenderDocで確認するとき`make_no_dcomp`で作成しないとクラッシュする
        if (this->use_dcomp) {
            auto result = D3D11::make(native_handle.hwnd, window_size)
                              .add_message("Rendererの初期化に失敗しました");
            if (result.is_err()) {
                return result.propagation(platform::RenderError::MakeError);
            }
            d3d11 = std::move(result).unwrap_mut();
        } else {
            auto result = D3D11::make_no_dcomp(native_handle.hwnd, window_size)
                              .add_message("Rendererの初期化に失敗しました");
            if (result.is_err()) {
                return result.propagation(platform::RenderError::MakeError);
            }
            d3d11 = std::move(result).unwrap_mut();
        }

        return std::make_shared<D3D11Renderer>(std::move(d3d11));
    }
} // namespace enishi::renderer::directx