#pragma once
#include <bypass_shell/shell_monitor.h>
#include <sub_system/ui/interface_ui.h>

namespace enishi::ui {
    class SelectionShell {
      public:
        static void draw(sub_system::IUI& ui, bypass_shell::ShellMonitor& monitor);
    };
} // namespace enishi::ui
