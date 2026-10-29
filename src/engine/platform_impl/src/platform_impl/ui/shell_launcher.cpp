#include "shell_launcher.h"
#include <SDL3/SDL.h>
#include <filesystem>
#if defined(_WIN32)
#include "shell_launcher_windows.h"
#elif defined(__APPLE__)
#include "shell_launcher_macos.h"
#elif defined(__linux__)
#include "shell_launcher_linux.h"
#else
#error Unsupported shell launcher platform
#endif
namespace enishi::platform_impl {
    std::expected<void, std::string> launch_shell_terminal() {
        const auto* base = SDL_GetBasePath();
        if (base == nullptr) {
            return std::unexpected(std::string(SDL_GetError()));
        }
        const auto executable =
            std::filesystem::u8path(base) / native_shell_launcher::EXECUTABLE_NAME;
        std::error_code error;
        const bool exists = std::filesystem::is_regular_file(executable, error);
        if (error.value() != 0) {
            return std::unexpected("Cannot access enishi-shell: " + error.message());
        }
        if (!exists) {
            return std::unexpected("enishi-shell was not found beside the application.");
        }
        return native_shell_launcher::launch(executable);
    }
} // namespace enishi::platform_impl
