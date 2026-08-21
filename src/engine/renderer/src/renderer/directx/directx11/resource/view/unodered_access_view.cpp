#include "unodered_access_view.h"

namespace enishi::renderer::directx {
    D3D11UnorderedAccessView::D3D11UnorderedAccessView(
        const types::RenderHandle handle, const types::ImageViewDescription description) {
    }
    types::ImageViewDescription D3D11UnorderedAccessView::get_description(void) const noexcept {
        return types::ImageViewDescription();
    }
    types::RenderHandle D3D11UnorderedAccessView::get_handle(void) const noexcept {
        return types::RenderHandle();
    }
    std::uint32_t D3D11UnorderedAccessView::mip_level(void) const noexcept {
        return std::uint32_t();
    }
} // namespace enishi::renderer::directx