#pragma once
#include <string>
#include <sub_system/model/interface_model_selection.h>
#include <sub_system/ui/interface_ui.h>
namespace enishi::ui {
    class SelectionModels final {
      private:
        std::string error_message;

      public:
        void reset_error();
        void draw(sub_system::IUI& ui, sub_system::IModelSelection& models);

      private:
        void draw_model(sub_system::IUI& ui,
            sub_system::IModelSelection& models,
            const sub_system::ModelEntry& model);
    };
} // namespace enishi::ui
