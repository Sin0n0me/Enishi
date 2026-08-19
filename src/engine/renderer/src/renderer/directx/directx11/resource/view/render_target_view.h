#pragma once
#include <platform/renderer/interface_image_view.h>

namespace enishi::renderer::directx {
    class UnorderedAccessView : public platform::IUnorderedAccessView {
      private:
        types::ImageViewDescription description;
        types::RenderHandle handle;

      public:
        explicit UnorderedAccessView(
            const types::RenderHandle handle, const types::ImageViewDescription description);
    };
} // namespace enishi::renderer::directx
