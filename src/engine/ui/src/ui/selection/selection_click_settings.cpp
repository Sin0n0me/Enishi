#include "selection_click_settings.h"

namespace enishi::ui {
    void SelectionClickSettings::draw(
        sub_system::IUI& ui, types::ModelClickSettings& click_settings) {
        ui.text("クリック状態");
        ui.text("Clicks: " + std::to_string(click_settings.click_count));
        ui.text(click_settings.last_target);
        ui.text(click_settings.last_result);
        const auto draw_mode = [&click_settings, &ui](
                                   const char* label, types::ModelClickMode mode) {
            ui.begin_disabled(click_settings.mode == mode);
            if (ui.button(label)) {
                click_settings.mode = mode;
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
