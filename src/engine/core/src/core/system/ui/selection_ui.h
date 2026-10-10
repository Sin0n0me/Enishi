#pragma once
#include <memory>
#include <string>
#include <sub_system/model/interface_model_selection.h>
#include <sub_system/ui/interface_ui.h>

namespace enishi::core {
    class SelectionUI {
      private:
        enum class Page { Actions, Models };

      private:
        std::shared_ptr<sub_system::IModelSelection> models;
        Page page = Page::Actions;
        std::string error_message;
        bool exit_requested = false;

      public:
        explicit SelectionUI(std::shared_ptr<sub_system::IModelSelection> models);
        void update(sub_system::IUI& ui);
        [[nodiscard]] bool should_close(void) const;

      private:
        void draw_actions(sub_system::IUI& ui);
        void draw_models(sub_system::IUI& ui);
        void draw_model(sub_system::IUI& ui, const sub_system::ModelEntry& model);
    };
} // namespace enishi::core
