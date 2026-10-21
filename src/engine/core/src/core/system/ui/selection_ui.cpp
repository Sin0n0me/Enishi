#include "selection_ui.h"
#include <foundation/log/logger.h>
#include <utility>

namespace enishi::core {
    SelectionUI::SelectionUI(std::shared_ptr<sub_system::IModelSelection> models,
        std::shared_ptr<ModelClickSettings> click_settings,
        std::shared_ptr<sub_system::IScriptEngine> scripts,
        std::shared_ptr<sub_system::RenderSettings> render_settings)
        : models(std::move(models))
        , click_settings(std::move(click_settings))
        , scripts(std::move(scripts))
        , render_settings(std::move(render_settings)) {
    }

    bool SelectionUI::is_exit_requested(void) const {
        return this->exit_requested;
    }

    void SelectionUI::update(sub_system::IUI& ui) {
        constexpr float ACTION_WIDTH = 240.0f;
        constexpr float ACTION_HEIGHT = 220.0f;
        constexpr float SETTINGS_WIDTH = 560.0f;
        constexpr float SETTINGS_HEIGHT = 360.0f;
        constexpr float SCRIPT_WIDTH = 640.0f;
        constexpr float SCRIPT_HEIGHT = 460.0f;
        float width = ACTION_WIDTH;
        float height = ACTION_HEIGHT;
        if (this->page == Page::Script) {
            width = SCRIPT_WIDTH;
            height = SCRIPT_HEIGHT;
        } else if (this->page != Page::Actions) {
            width = SETTINGS_WIDTH;
            height = SETTINGS_HEIGHT;
        }
        if (!ui.begin_context_popup("selection", width, height)) {
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
            case Page::ClickSettings:
                this->draw_click_settings(ui);
                break;
            case Page::Settings:
                this->draw_settings(ui);
                break;
            case Page::Script:
                this->draw_script(ui);
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
        if (ui.button("クリック設定")) {
            this->page = Page::ClickSettings;
        }
        if (ui.button("スクリプト")) {
            this->page = Page::Script;
        }
        if (ui.button("設定")) {
            this->page = Page::Settings;
        }
        if (ui.button("終了")) {
            this->exit_requested = true;
        }
    }

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

    void SelectionUI::draw_script(sub_system::IUI& ui) {
        if (ui.button("戻る")) {
            this->page = Page::Actions;
            return;
        }
        ui.text("引数なしの main() を実行します");
        ui.input_text_multiline("スクリプト", this->script_source, false);
        if (ui.button("実行")) {
            this->script_error.clear();
            this->script_status.clear();
            auto result = this->scripts->execute(this->script_source);
            if (result.is_err()) {
                this->script_error = result.unwrap_err().get_message();
            } else {
                this->script_status = "実行しました";
            }
        }
        ui.text(this->script_status);
        ui.input_text_multiline("スクリプトエラー", this->script_error, true);
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
        const auto draw_mode = [this, &ui](const char* label, ModelClickMode mode) {
            ui.begin_disabled(this->click_settings->mode == mode);
            if (ui.button(label)) {
                this->click_settings->mode = mode;
            }
            ui.end_disabled();
        };
        draw_mode("なし", ModelClickMode::None);
        draw_mode("触る", ModelClickMode::Touch);
        draw_mode("押す", ModelClickMode::Push);
        ui.text("触る: ボーンに設定されたアニメーションを再生");
        ui.text("押す: クリックした部位に衝撃を加える");
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
