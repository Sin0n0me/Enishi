#pragma once
#include "selection_models.h"
#include "selection_script.h"
#include <engine_types/input/model_click_settings.h>
#include <memory>
#include <sub_system/model/interface_model_selection.h>
#include <sub_system/render/render_settings.h>
#include <sub_system/script/interface_script_engine.h>
#include <sub_system/ui/interface_ui.h>

namespace enishi::ui {
    class SelectionUI {
      private:
        enum class Page { Actions, Models, ClickSettings, Script, Settings };

      private:
        std::shared_ptr<sub_system::IModelSelection> models;
        std::shared_ptr<types::ModelClickSettings> click_settings;
        std::shared_ptr<sub_system::IScriptEngine> scripts;
        std::shared_ptr<sub_system::RenderSettings> render_settings;
        SelectionModels models_page;
        SelectionScript script_page;
        Page page = Page::Actions;
        bool exit_requested = false;

      public:
        SelectionUI(std::shared_ptr<sub_system::IModelSelection> models,
            std::shared_ptr<types::ModelClickSettings> click_settings,
            std::shared_ptr<sub_system::IScriptEngine> scripts,
            std::shared_ptr<sub_system::RenderSettings> render_settings);
        void update(sub_system::IUI& ui);
        [[nodiscard]] bool is_exit_requested(void) const;

      private:
        void draw_actions(sub_system::IUI& ui);
        void draw_page(sub_system::IUI& ui);
    };
} // namespace enishi::ui
