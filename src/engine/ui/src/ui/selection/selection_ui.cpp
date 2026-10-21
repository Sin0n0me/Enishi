#include "selection_ui.h"
#include <foundation/log/logger.h>
#include <utility>

namespace enishi::ui {
    SelectionUI::SelectionUI(std::shared_ptr<sub_system::IModelSelection> models,
        std::shared_ptr<types::ModelClickSettings> click_settings,
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

} // namespace enishi::ui
