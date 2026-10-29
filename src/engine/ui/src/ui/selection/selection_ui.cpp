#include "selection_ui.h"
#include "selection_click_settings.h"
#include "selection_settings.h"
#include <utility>

namespace enishi::ui {
    SelectionUI::SelectionUI(std::shared_ptr<sub_system::IModelSelection> models,
        std::shared_ptr<types::ModelClickSettings> click_settings,
        std::shared_ptr<sub_system::IScriptEngine> scripts,
        std::shared_ptr<sub_system::RenderSettings> render_settings,
        std::shared_ptr<bypass_shell::ShellMonitor> shell_monitor,
        ShellLauncher launch_shell)
        : models(std::move(models))
        , click_settings(std::move(click_settings))
        , scripts(std::move(scripts))
        , render_settings(std::move(render_settings))
        , shell_monitor(std::move(shell_monitor))
        , launch_shell(std::move(launch_shell)) {
    }

    bool SelectionUI::is_exit_requested(void) const {
        return this->exit_requested;
    }

    void SelectionUI::update(sub_system::IUI& ui) {
        constexpr float ACTION_WIDTH = 240.0f;
        constexpr float ACTION_HEIGHT = 260.0f;
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
            this->models_page.reset_error();
        }
        this->draw_page(ui);
        ui.end_popup();
    }

    void SelectionUI::draw_actions(sub_system::IUI& ui) {
        if (this->shell_monitor != nullptr) {
            const bool can_launch =
                static_cast<bool>(this->launch_shell) && this->shell_monitor->is_listening();
            ui.begin_disabled(!can_launch);
            if (ui.button("シェルを起動") && can_launch) {
                const auto result = this->launch_shell();
                this->shell_error = result.has_value() ? std::string{} : result.error();
            }
            ui.end_disabled();
            if (!this->shell_error.empty()) {
                ui.text(this->shell_error);
            }
            if (!this->shell_monitor->is_listening()) {
                ui.text("シェルの接続受付が開始されていません。");
                if (!this->shell_monitor->last_error().empty()) {
                    ui.text(this->shell_monitor->last_error());
                }
            }
        }
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

    void SelectionUI::draw_page(sub_system::IUI& ui) {
        if (this->page != Page::Actions && ui.button("戻る")) {
            this->page = Page::Actions;
            return;
        }
        switch (this->page) {
            case Page::Actions:
                this->draw_actions(ui);
                break;
            case Page::ClickSettings:
                SelectionClickSettings::draw(ui, *this->click_settings);
                break;
            case Page::Settings:
                SelectionSettings::draw(ui, *this->render_settings);
                break;
            case Page::Script:
                this->script_page.draw(ui, *this->scripts);
                break;
            case Page::Models:
                this->models_page.draw(ui, *this->models);
                break;
        }
    }
} // namespace enishi::ui
