#include "depth_stencil_view.h"

namespace enishi::renderer::directx {
    D3D11DepthStencilView::D3D11DepthStencilView(
        const types::RenderHandle handle, const types::ImageViewDescription description)
        : handle(handle)
        , description(description) {
    }
    types::ImageViewDescription D3D11DepthStencilView::get_description(void) const noexcept {
        return this->description;
    }
    types::RenderHandle D3D11DepthStencilView::get_handle(void) const noexcept {
        return this->handle;
    }
    float D3D11DepthStencilView::clear_depth(void) const noexcept {
        return 1.0f;
    }
    std::uint8_t D3D11DepthStencilView::clear_stencil(void) const noexcept {
        return 0;
    }
    void D3D11DepthStencilView::set_clear_depth(const float depth) noexcept {
    }
    void D3D11DepthStencilView::set_clear_stencil(const std::uint8_t stencil) noexcept {
    }
} // namespace enishi::renderer::directx