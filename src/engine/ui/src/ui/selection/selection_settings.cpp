#include "selection_settings.h"

namespace enishi::ui {
    void SelectionSettings::draw(sub_system::IUI& ui, sub_system::RenderSettings& render_settings) {
        ui.text("設定");
        ui.text("体の影（モデル自身に落ちる影）");
        const auto draw_toggle = [&render_settings, &ui](const char* label, bool enabled) {
            ui.begin_disabled(render_settings.self_shadow_enabled == enabled);
            if (ui.button(label)) {
                render_settings.self_shadow_enabled = enabled;
            }
            ui.end_disabled();
        };
        draw_toggle("オン", true);
        ui.same_line();
        draw_toggle("オフ", false);
    }

} // namespace enishi::ui
