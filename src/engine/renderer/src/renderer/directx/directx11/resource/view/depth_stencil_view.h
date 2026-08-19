#pragma once
#include <platform/renderer/interface_image_view.h>

namespace enishi::renderer::directx {
    class DepthStencilView : public platform::IDepthStencilView {
      private:
        types::ImageViewDescription description;
        types::RenderHandle handle;

      public:
        explicit DepthStencilView(
            const types::RenderHandle handle, const types::ImageViewDescription description);
    };
} // namespace enishi::renderer::directx
