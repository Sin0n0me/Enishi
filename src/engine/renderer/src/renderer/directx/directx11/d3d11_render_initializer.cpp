#include "d3d11_render_initializer.h"

namespace enishi::renderer::directx {
    foundation::Result<std::shared_ptr<D3D11Renderer>, platform::RenderError>
    D3D11RenderInitializer::init(
        const platform::WindowHandle& window_handle, const types::WindowSize& window_size) {
        if (window_handle.tag != platform::WindowSystem::Windows) {
            return foundation::Error(
                platform::RenderError::MakeError, "Windows以外では動作しません");
        }

        const auto native_handle = window_handle.native_handle.windows;

        auto d3d11 = D3D11::make(native_handle.hwnd, window_size);
        if (d3d11.is_err()) {
            return d3d11.add_message("Rendererの初期化に失敗しました")
                .propagation(platform::RenderError::MakeError);
        }

        return std::make_shared<D3D11Renderer>(std::move(d3d11.value()));
    }
} // namespace enishi::renderer::directx