#include "ui_system.h"
#include <utility>

namespace enishi::core {
    UISystem::UISystem(std::shared_ptr<sub_system::IUI> ui, Content content)
        : ui(std::move(ui))
        , content(std::move(content)) {
    }

    void UISystem::pre_update(void) {
    }

    void UISystem::update(const types::DeltaTime& delta_time) {
        this->ui->begin_frame();
        if (this->content != nullptr) {
            this->content(*this->ui);
        }
    }

    void UISystem::post_update(void) {
        // Debug overlays use collision bounds finalized during post_update.
        this->ui->end_frame();
    }

    void UISystem::render(void) const {
        // RenderSystem submits the UI after the scene and before presenting.
    }
} // namespace enishi::core
