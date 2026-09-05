#include "shader_resource_view.h"

namespace enishi::renderer::directx {
    D3D11ShaderResourceView::D3D11ShaderResourceView(
        const types::RenderHandle handle, const types::ImageViewDescription description)
        : handle(handle)
        , description(description) {
    }
    types::ImageViewDescription D3D11ShaderResourceView::get_description(void) const noexcept {
        return this->description;
    }
    types::RenderHandle D3D11ShaderResourceView::get_handle(void) const noexcept {
        return this->handle;
    }
    types::SamplerStateDescription D3D11ShaderResourceView::sampler_description(void) const noexcept {
        return types::SamplerStateDescription();
    }
} // namespace enishi::renderer::directx