#include "selection_shell.h"
#include <utility>

namespace enishi::ui {
    namespace {
        const char* label(bypass_shell::ShellEventKind kind) {
            using bypass_shell::ShellEventKind;
            switch (kind) {
                case ShellEventKind::Started:
                    return "start";
                case ShellEventKind::Input:
                    return "input";
                case ShellEventKind::Stdout:
                    return "stdout";
                case ShellEventKind::Stderr:
                    return "stderr";
                case ShellEventKind::Exited:
                    return "exit";
                case ShellEventKind::Disconnected:
                    return "disconnect";
            }
            return "unknown";
        }
        std::string visible(std::string_view bytes) {
            constexpr unsigned char FIRST_PRINTABLE = 32;
            constexpr unsigned char LAST_PRINTABLE = 126;
            constexpr unsigned HEX_SHIFT = 4;
            constexpr unsigned HEX_MASK = 15;
            constexpr std::string_view HEX = "0123456789abcdef";
            std::string result;
            for (const unsigned char byte : bytes) {
                if ((!(byte < FIRST_PRINTABLE) && !(byte > LAST_PRINTABLE)) || byte == '\n' ||
                    byte == '\t') {
                    result.push_back(static_cast<char>(byte));
                } else {
                    // Preserve arbitrary code pages and control bytes without interpreting terminal
                    // escapes.
                    result.append("\\x");
                    result.push_back(HEX[byte >> HEX_SHIFT]);
                    result.push_back(HEX[byte & HEX_MASK]);
                }
            }
            return result;
        }
    } // namespace
    SelectionShell::SelectionShell(Launcher launcher)
        : launcher(std::move(launcher)) {
    }
    void SelectionShell::draw(sub_system::IUI& ui, bypass_shell::ShellMonitor& monitor) {
        const bool can_launch = static_cast<bool>(this->launcher) && monitor.is_listening();
        ui.begin_disabled(!can_launch);
        if (ui.button("シェルを起動") && can_launch) {
            const auto result = this->launcher();
            this->launch_message =
                result.has_value() ? "ターミナルの起動を要求しました。" : result.error();
        }
        ui.end_disabled();
        if (!this->launch_message.empty()) {
            ui.text(this->launch_message);
        }
        if (!monitor.is_listening()) {
            ui.text("シェルの接続受付が開始されていません。");
        }
        ui.text("入力・出力はセッション単位で表示します。非ASCIIバイトは16進表記です。");
        if (!monitor.last_error().empty()) {
            ui.text(monitor.last_error());
        }
        if (ui.button("履歴を消去")) {
            monitor.clear_history();
        }
        std::string text;
        for (const auto& event : monitor.events()) {
            text += "[" + std::to_string(event.session) + " " + label(event.kind) + "] " +
                    visible(event.data) + "\n";
        }
        ui.input_text_multiline("シェル入出力", text, true);
    }
} // namespace enishi::ui
