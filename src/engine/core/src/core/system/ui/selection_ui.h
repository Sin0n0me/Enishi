#pragma once
#include <core/system/collision/model_click_settings.h>
#include <memory>
#include <string>
#include <sub_system/model/interface_model_selection.h>
#include <sub_system/render/render_settings.h>
#include <sub_system/script/interface_script_engine.h>
#include <sub_system/ui/interface_ui.h>

namespace enishi::core {
    class SelectionUI {
      private:
        enum class Page { Actions, Models, ClickSettings, Script, Settings };

      private:
        std::shared_ptr<sub_system::IModelSelection> models;
        std::shared_ptr<ModelClickSettings> click_settings;
        std::shared_ptr<sub_system::IScriptEngine> scripts;
        std::shared_ptr<sub_system::RenderSettings> render_settings;
        std::string script_source = "fn main() -> i32 {\n    42\n}\n";
        std::string script_error;
        std::string script_status;
        Page page = Page::Actions;
        std::string error_message;
        bool exit_requested = false;

      public:
        SelectionUI(std::shared_ptr<sub_system::IModelSelection> models,
            std::shared_ptr<ModelClickSettings> click_settings,
            std::shared_ptr<sub_system::IScriptEngine> scripts,
            std::shared_ptr<sub_system::RenderSettings> render_settings);
        void update(sub_system::IUI& ui);
        [[nodiscard]] bool should_close(void) const;

      private:
        void draw_actions(sub_system::IUI& ui);
        void draw_click_settings(sub_system::IUI& ui);
        void draw_settings(sub_system::IUI& ui);
        void draw_script(sub_system::IUI& ui);
        void draw_models(sub_system::IUI& ui);
        void draw_model(sub_system::IUI& ui, const sub_system::ModelEntry& model);
    };
} // namespace enishi::core
