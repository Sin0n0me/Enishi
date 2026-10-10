#pragma once
#include <core/system/collision/model_click_settings.h>
#include <memory>
#include <string>
#include <sub_system/model/interface_model_selection.h>
#include <sub_system/ui/interface_ui.h>

namespace enishi::core {
    class SelectionUI {
      private:
        enum class Page { Actions, Models, ClickSettings };

      private:
        std::shared_ptr<sub_system::IModelSelection> models;
        std::shared_ptr<ModelClickSettings> click_settings;
        Page page = Page::Actions;
        std::string error_message;
        bool exit_requested = false;

      public:
        SelectionUI(std::shared_ptr<sub_system::IModelSelection> models,
            std::shared_ptr<ModelClickSettings> click_settings);
        void update(sub_system::IUI& ui);
        [[nodiscard]] bool should_close(void) const;

      private:
        void draw_actions(sub_system::IUI& ui);
        void draw_click_settings(sub_system::IUI& ui);
        void draw_models(sub_system::IUI& ui);
        void draw_model(sub_system::IUI& ui, const sub_system::ModelEntry& model);
    };
} // namespace enishi::core
