#include "selection_ui.h"
#include <foundation/log/logger.h>

namespace enishi::ui {
    void SelectionUI::draw_settings(sub_system::IUI& ui) {
        if (ui.button("戻る")) {
            this->page = Page::Actions;
            return;
        }
        ui.text("設定");
        ui.text("体の影（モデル自身に落ちる影）");
        const auto draw_toggle = [this, &ui](const char* label, bool enabled) {
            ui.begin_disabled(this->render_settings->self_shadow_enabled == enabled);
            if (ui.button(label)) {
                this->render_settings->self_shadow_enabled = enabled;
            }
            ui.end_disabled();
        };
        draw_toggle("オン", true);
        ui.same_line();
        draw_toggle("オフ", false);
    }

    void SelectionUI::draw_click_settings(sub_system::IUI& ui) {
        if (ui.button("戻る")) {
            this->page = Page::Actions;
            return;
        }
        ui.text("クリック状態");
        ui.text("Clicks: " + std::to_string(this->click_settings->click_count));
        ui.text(this->click_settings->last_target);
        ui.text(this->click_settings->last_result);
        const auto draw_mode = [this, &ui](const char* label, types::ModelClickMode mode) {
            ui.begin_disabled(this->click_settings->mode == mode);
            if (ui.button(label)) {
                this->click_settings->mode = mode;
            }
            ui.end_disabled();
        };
        draw_mode("なし", types::ModelClickMode::None);
        draw_mode("触る", types::ModelClickMode::Touch);
        draw_mode("押す", types::ModelClickMode::Push);
        ui.text("触る: ボーンに設定されたアニメーションを再生");
        ui.text("押す: クリックした部位に衝撃を加える");
    }

} // namespace enishi::ui
