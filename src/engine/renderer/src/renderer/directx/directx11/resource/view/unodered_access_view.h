#pragma once
#include <platform/renderer/interface_image_view.h>

namespace enishi::renderer::directx {
    class D3D11UnorderedAccessView : public platform::IUnorderedAccessView {
      private:
        types::ImageViewDescription description;
        types::RenderHandle handle;

      public:
        explicit D3D11UnorderedAccessView(
            const types::RenderHandle handle, const types::ImageViewDescription description);

        types::ImageViewDescription get_description(void) const noexcept override;
        types::RenderHandle get_handle(void) const noexcept override;
        std::uint32_t mip_level(void) const noexcept override;
    };
} // namespace enishi::renderer::directx
