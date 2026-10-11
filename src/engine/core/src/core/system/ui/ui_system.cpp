#include "ui_system.h"
#include <utility>

namespace enishi::core {
    UISystem::UISystem(
        std::shared_ptr<sub_system::IUI> ui, Content content, CloseCondition close_condition)
        : ui(std::move(ui))
        , content(std::move(content))
        , close_condition(std::move(close_condition)) {
    }

    bool UISystem::should_close(void) {
        return this->close_condition != nullptr && this->close_condition();
    }

    void UISystem::pre_update(void) {
    }

    void UISystem::update(const types::DeltaTime& delta_time) {
        this->ui->begin_frame();
        if (this->content != nullptr) {
            this->content(*this->ui);
        }
        this->ui->end_frame();
    }

    void UISystem::post_update(void) {
    }

    void UISystem::render(void) const {
        // RenderSystem submits the UI after the scene and before presenting.
    }
} // namespace enishi::core
