#include "posix_terminal_input.h"
#include <array>
#include <cerrno>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

namespace enishi::platform_impl {
    namespace {
        constexpr std::size_t INPUT_CHUNK = 4096;
    } // namespace
    struct PosixTerminalInput::Impl {
        termios original{};
        bool restore = false;
        std::string error;
        Impl();
        ~Impl();
    };
    PosixTerminalInput::Impl::Impl() {
        if (isatty(STDIN_FILENO) != 1 || isatty(STDOUT_FILENO) != 1) {
            return;
        }
        if (tcgetattr(STDIN_FILENO, &this->original) == -1) {
            this->error = "Cannot read terminal mode";
            return;
        }
        auto raw = this->original;
        cfmakeraw(&raw);
        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == -1) {
            this->error = "Cannot enable raw terminal input";
            return;
        }
        this->restore = true;
    }
    PosixTerminalInput::Impl::~Impl() {
        if (this->restore) {
            tcsetattr(STDIN_FILENO, TCSANOW, &this->original);
        }
    }
    PosixTerminalInput::PosixTerminalInput()
        : impl(std::make_unique<Impl>()) {
    }
    PosixTerminalInput::~PosixTerminalInput() = default;
    std::expected<std::optional<std::string>, std::string> PosixTerminalInput::read() {
        if (!this->impl->error.empty()) {
            return std::unexpected(this->impl->error);
        }
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
