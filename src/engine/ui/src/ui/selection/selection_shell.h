#pragma once
#include <bypass_shell/shell_monitor.h>
#include <expected>
#include <functional>
#include <string>
#include <sub_system/ui/interface_ui.h>

namespace enishi::ui {
    class SelectionShell {
      public:
        using Launcher = std::function<std::expected<void, std::string>()>;

      private:
        Launcher launcher;
        std::string launch_message;

      public:
        explicit SelectionShell(Launcher launcher = {});
        void draw(sub_system::IUI& ui, bypass_shell::ShellMonitor& monitor);
    };
} // namespace enishi::ui
