#pragma once
#include <platform/renderer/view/interface_image_view.h>

namespace enishi::renderer::directx {
    class D3D11DepthStencilView : public platform::IDepthStencilView {
      private:
        types::ImageViewDescription description;
        types::RenderHandle handle;

      public:
        explicit D3D11DepthStencilView(
            const types::RenderHandle handle, const types::ImageViewDescription description);

        types::ImageViewDescription get_description(void) const noexcept override;
        types::RenderHandle get_handle(void) const noexcept override;
        float clear_depth(void) const noexcept override;
        std::uint8_t clear_stencil(void) const noexcept override;
        void set_clear_depth(const float depth) noexcept override;
        void set_clear_stencil(const std::uint8_t stencil) noexcept override;
    };
} // namespace enishi::renderer::directx
