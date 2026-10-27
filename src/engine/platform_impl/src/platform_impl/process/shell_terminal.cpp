#include "shell_terminal.h"
#if defined(_WIN32)
#include "windows/windows_shell_terminal.h"
#elif defined(__linux__)
#include "linux/linux_shell_terminal.h"
#else
#error Unsupported shell platform
#endif
namespace enishi::platform_impl {
    void initialize_shell_terminal() {
        native_terminal::initialize_shell_terminal();
    }
    bool shell_interrupted() {
        return native_terminal::shell_interrupted();
    }
    std::string_view shell_program_name() {
        return native_terminal::shell_program_name();
    }
} // namespace enishi::platform_impl
