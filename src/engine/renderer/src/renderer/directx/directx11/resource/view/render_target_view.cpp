#include "render_target_view.h"

namespace enishi::renderer::directx {
    RenderTargetView::RenderTargetView(
        const types::RenderHandle handle, const types::ImageViewDescription description)
        : handle(handle)
        , description(description)
        , clear_color(glm::vec4{0, 0, 0, 0}) {
    }

    types::ImageViewDescription RenderTargetView::get_description(void) const noexcept {
        return this->description;
    }

    types::RenderHandle RenderTargetView::get_handle(void) const noexcept {
        return this->handle;
    }

    glm::vec4 RenderTargetView::get_clear_color(void) const noexcept {
        return this->clear_color;
    }

    void RenderTargetView::set_clear_color(glm::vec4 color) noexcept {
        this->clear_color = color;
    }
} // namespace enishi::renderer::directx