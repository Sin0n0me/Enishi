#include "posix_terminal_input.h"
#include <array>
#include <cerrno>
#include <poll.h>
#include <unistd.h>

namespace enishi::platform_impl {
    namespace {
        constexpr std::size_t INPUT_CHUNK = 4096;
    } // namespace
    struct PosixTerminalInput::Impl {};
    PosixTerminalInput::PosixTerminalInput()
        : impl(std::make_unique<Impl>()) {
    }
    PosixTerminalInput::~PosixTerminalInput() = default;
    std::expected<std::optional<std::string>, std::string> PosixTerminalInput::read() {
        std::array<char, INPUT_CHUNK> buffer{};
        pollfd input{STDIN_FILENO, POLLIN, 0};
        const auto ready = ::poll(&input, 1, 0);
        if (ready == 0 || (ready < 0 && errno == EINTR)) {
            return std::optional<std::string>{};
        }
        if (ready < 0) {
            return std::unexpected("Cannot poll terminal input");
        }
        const auto count = ::read(STDIN_FILENO, buffer.data(), buffer.size());
        if (count < 0) {
            if (errno == EINTR) {
                return std::optional<std::string>{};
            }
            return std::unexpected("Cannot read terminal input");
        }
        return std::string(buffer.data(), static_cast<std::size_t>(count));
    }
} // namespace enishi::platform_impl
