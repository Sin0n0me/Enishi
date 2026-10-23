#include "selection_models.h"
#include <foundation/log/logger.h>

namespace enishi::ui {
    void SelectionModels::reset_error() {
        this->error_message.clear();
    }

    void SelectionModels::draw(sub_system::IUI& ui, sub_system::IModelSelection& models) {
        const auto entries = models.get_models();
        if (entries.empty()) {
            ui.text("モデルが見つかりません");
        }
        for (const auto& model : entries) {
            this->draw_model(ui, models, model);
        }
        if (!this->error_message.empty()) {
            ui.text(this->error_message);
        }
    }

    void SelectionModels::draw_model(sub_system::IUI& ui,
        sub_system::IModelSelection& models,
        const sub_system::ModelEntry& model) {
        ui.push_id(model.id.c_str());
        ui.text(model.filename);
        ui.tooltip(model.id);
        ui.same_line();
        ui.begin_disabled(model.visible);
        if (ui.button("表示")) {
            const auto result = models.show_model(model.id);
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
            models.hide_model(model.id);
            this->error_message.clear();
        }
        ui.end_disabled();
        ui.pop_id();
    }
} // namespace enishi::ui
