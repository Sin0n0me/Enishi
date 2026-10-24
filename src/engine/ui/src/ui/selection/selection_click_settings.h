#pragma once
#include <engine_types/input/model_click_settings.h>
#include <sub_system/ui/interface_ui.h>

namespace enishi::ui {
    class SelectionClickSettings final {
      public:
        static void draw(sub_system::IUI& ui, types::ModelClickSettings& click_settings);
    };
} // namespace enishi::ui
