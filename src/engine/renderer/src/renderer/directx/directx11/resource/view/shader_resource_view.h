#pragma once
#include <platform/renderer/view/interface_image_view.h>

namespace enishi::renderer::directx {
    class D3D11ShaderResourceView : public platform::IShaderResourceView {
      private:
        types::ImageViewDescription description;
        types::RenderHandle handle;

      public:
        explicit D3D11ShaderResourceView(
            const types::RenderHandle handle, const types::ImageViewDescription description);

        types::ImageViewDescription get_description(void) const noexcept override;
        types::RenderHandle get_handle(void) const noexcept override;
        types::SamplerStateDescription sampler_description(void) const noexcept override;
    };
} // namespace enishi::renderer::directx
