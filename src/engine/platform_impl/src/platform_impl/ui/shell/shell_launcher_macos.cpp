#include "shell_launcher_macos.h"
#include "terminal_process.h"
namespace enishi::platform_impl::native_shell_launcher {
    std::expected<void, std::string> launch(const std::filesystem::path& executable) {
        const auto path = executable.string();
        const char* arguments[]{"/usr/bin/open", "-a", "Terminal", "--", path.c_str(), nullptr};
        return launch_terminal_process(arguments);
    }
} // namespace enishi::platform_impl::native_shell_launcher
