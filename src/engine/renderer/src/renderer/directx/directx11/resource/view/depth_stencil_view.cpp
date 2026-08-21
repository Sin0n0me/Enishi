#include "depth_stencil_view.h"

namespace enishi::renderer::directx {
    D3D11DepthStencilView::D3D11DepthStencilView(
        const types::RenderHandle handle, const types::ImageViewDescription description) {
    }
    types::ImageViewDescription D3D11DepthStencilView::get_description(void) const noexcept {
        return types::ImageViewDescription();
    }
    types::RenderHandle D3D11DepthStencilView::get_handle(void) const noexcept {
        return types::RenderHandle();
    }
    float D3D11DepthStencilView::clear_depth(void) const noexcept {
        return 0.0f;
    }
    std::uint8_t D3D11DepthStencilView::clear_stencil(void) const noexcept {
        return std::uint8_t();
    }
    void D3D11DepthStencilView::set_clear_depth(const float depth) noexcept {
    }
    void D3D11DepthStencilView::set_clear_stencil(const std::uint8_t stencil) noexcept {
    }
} // namespace enishi::renderer::directx