#include "terminal_input.h"
#if defined(_WIN32)
#include "windows/windows_terminal_input.h"
#elif defined(__linux__)
#include "linux/linux_terminal_input.h"
#else
#error Unsupported shell platform
#endif
namespace enishi::platform_impl {
    struct TerminalInput::Impl {
        NativeTerminalInput input;
    };
    TerminalInput::TerminalInput()
        : impl(std::make_unique<Impl>()) {
    }
    TerminalInput::~TerminalInput() = default;
    std::expected<std::optional<std::string>, std::string> TerminalInput::read() {
        return this->impl->input.read();
    }
} // namespace enishi::platform_impl
