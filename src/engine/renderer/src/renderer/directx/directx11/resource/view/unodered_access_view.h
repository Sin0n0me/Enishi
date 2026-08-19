#pragma once
#include <platform/renderer/interface_image_view.h>

namespace enishi::renderer::directx {
    class RenderTargetView : public platform::IRenderTargetView {
      private:
        types::ImageViewDescription description;
        types::RenderHandle handle;
        glm::vec4 clear_color;

      public:
        explicit RenderTargetView(
            const types::RenderHandle handle, const types::ImageViewDescription description);

        types::ImageViewDescription get_description(void) const noexcept override;
        types::RenderHandle get_handle(void) const noexcept override;
        glm::vec4 get_clear_color(void) const noexcept override;
        void set_clear_color(glm::vec4 color) noexcept override;
    };
} // namespace enishi::renderer::directx
