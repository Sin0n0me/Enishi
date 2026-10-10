#include "selection_ui.h"
#include <foundation/log/logger.h>
#include <utility>

namespace enishi::core {
    SelectionUI::SelectionUI(std::shared_ptr<sub_system::IModelSelection> models)
        : models(std::move(models)) {
    }

    bool SelectionUI::should_close(void) const {
        return this->exit_requested;
    }

    void SelectionUI::update(sub_system::IUI& ui) {
        if (!ui.begin_context_popup("selection")) {
            return;
        }
        if (ui.is_window_appearing()) {
            this->page = Page::Actions;
            this->error_message.clear();
        }
        switch (this->page) {
            case Page::Actions:
                this->draw_actions(ui);
                break;
            case Page::Models:
                this->draw_models(ui);
                break;
        }
        ui.end_popup();
    }

    void SelectionUI::draw_actions(sub_system::IUI& ui) {
        if (ui.button("モデルの切り替え")) {
            this->page = Page::Models;
        }
        if (ui.button("終了")) {
            this->exit_requested = true;
        }
    }

    void SelectionUI::draw_models(sub_system::IUI& ui) {
        if (ui.button("戻る")) {
            this->page = Page::Actions;
            return;
        }
        const auto entries = this->models->get_models();
        if (entries.empty()) {
            ui.text("モデルが見つかりません");
        }
        for (const auto& model : entries) {
            this->draw_model(ui, model);
        }
        if (!this->error_message.empty()) {
            ui.text(this->error_message);
        }
    }

    void SelectionUI::draw_model(sub_system::IUI& ui, const sub_system::ModelEntry& model) {
        ui.push_id(model.id.c_str());
        ui.text(model.filename);
        ui.tooltip(model.id);
        ui.same_line();
        ui.begin_disabled(model.visible);
        if (ui.button("表示")) {
            const auto result = this->models->show_model(model.id);
            this->error_message.clear();
            if (result.is_err()) {
                this->error_message = result.unwrap_err().get_message();
                foundation::Logger::error(result.unwrap_err().get_message());
            }
        }
        ui.end_disabled();
        ui.same_line();
        ui.begin_disabled(!model.visible);
        if (ui.button("非表示")) {
            this->models->hide_model(model.id);
            this->error_message.clear();
        }
        ui.end_disabled();
        ui.pop_id();
    }
} // namespace enishi::core
