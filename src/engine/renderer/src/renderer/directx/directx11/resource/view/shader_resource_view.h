#pragma once
#include <platform/renderer/interface_image_view.h>

namespace enishi::renderer::directx {
    class ShaderResourceView : public platform::IShaderResourceView {
      private:
        types::ImageViewDescription description;
        types::RenderHandle handle;

      public:
        explicit ShaderResourceView(
            const types::RenderHandle handle, const types::ImageViewDescription description);
    };
} // namespace enishi::renderer::directx
