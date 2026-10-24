#pragma once
#include <sub_system/render/render_settings.h>
#include <sub_system/ui/interface_ui.h>

namespace enishi::ui {
    class SelectionSettings final {
      public:
        static void draw(sub_system::IUI& ui, sub_system::RenderSettings& render_settings);
    };
} // namespace enishi::ui
