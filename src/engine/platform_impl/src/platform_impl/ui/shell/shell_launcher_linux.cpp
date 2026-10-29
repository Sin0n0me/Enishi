#include "shell_launcher_linux.h"
#include "terminal_process.h"
#include <array>
namespace enishi::platform_impl::native_shell_launcher {
    std::expected<void, std::string> launch(const std::filesystem::path& executable) {
        const auto path = executable.string();
        // Pass executable arguments separately; model/install paths are never shell source.
        constexpr std::array terminals{"x-terminal-emulator", "xterm"};
        std::string errors;
        for (const auto* terminal : terminals) {
            const char* arguments[]{terminal, "-e", path.c_str(), nullptr};
            auto result = launch_terminal_process(arguments);
            if (result.has_value()) {
                return {};
            }
            errors += std::string(terminal) + ": " + result.error() + "\n";
        }
        return std::unexpected(
            "Cannot open a terminal. Install x-terminal-emulator or xterm.\n" + errors);
    }
} // namespace enishi::platform_impl::native_shell_launcher
