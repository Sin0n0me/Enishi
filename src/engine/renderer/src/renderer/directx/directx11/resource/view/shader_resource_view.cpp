#include "shader_resource_view.h"

namespace enishi::renderer::directx {
    D3D11ShaderResourceView::D3D11ShaderResourceView(
        const types::RenderHandle handle, const types::ImageViewDescription description) {
    }
    types::ImageViewDescription D3D11ShaderResourceView::get_description(void) const noexcept {
        return types::ImageViewDescription();
    }
    types::RenderHandle D3D11ShaderResourceView::get_handle(void) const noexcept {
        return types::RenderHandle();
    }
    types::SamplerDescription D3D11ShaderResourceView::sampler_description(void) const noexcept {
        return types::SamplerDescription();
    }
} // namespace enishi::renderer::directx