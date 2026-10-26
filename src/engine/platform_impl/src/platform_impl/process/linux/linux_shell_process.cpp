#include "linux_shell_process.h"
#include <array>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <span>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace enishi::platform_impl {
    namespace {
        constexpr std::size_t IO_CHUNK_SIZE = 4096;
        using Native = int;
        constexpr Native INVALID_NATIVE = -1;
        void release(Native value) {
            ::close(value);
        }
        int native_error() {
            return errno;
        }
        class Handle {
          private:
            Native value = INVALID_NATIVE;

          public:
            inline Handle() = default;
            inline ~Handle() {
                this->reset();
            }
            Handle(const Handle&) = delete;
            Handle& operator=(const Handle&) = delete;
            inline Native get() const {
                return this->value;
            }
            inline void reset(Native next = INVALID_NATIVE) {
                if (this->value != INVALID_NATIVE) {
                    release(this->value);
                }
                this->value = next;
            }
        };
        std::unexpected<std::string> failure(std::string_view action) {
            return std::unexpected(
                std::string(action) + " (OS error " + std::to_string(native_error()) + ")");
        }
        bool make_pipe(Handle& read, Handle& write, bool socket_input = false) {
            std::array<int, 2> descriptors{};
            const auto status =
                socket_input
                    ? socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, descriptors.data())
                    : pipe2(descriptors.data(), O_CLOEXEC);
            if (status != 0) {
                return false;
            }
            read.reset(descriptors[0]);
            write.reset(descriptors[1]);
            return true;
        }
    } // namespace
    struct LinuxShellProcess::Impl {
        Handle input;
        Handle output;
        Handle error;
        std::optional<int> status;
        pid_t pid = -1;
        inline ~Impl() {
            this->input.reset();
            if (this->pid > 0) {
                kill(-this->pid, SIGKILL);
                if (!this->status.has_value()) {
                    while (waitpid(this->pid, nullptr, 0) < 0 && errno == EINTR) {
                    }
                }
            }
        }
    };
    LinuxShellProcess::LinuxShellProcess()
        : impl(std::make_unique<Impl>()) {
    }
    LinuxShellProcess::~LinuxShellProcess() = default;

    platform::ProcessResult<void> LinuxShellProcess::start() {
        this->impl = std::make_unique<Impl>();
        Handle child_input;
        Handle child_output;
        Handle child_error;
        if (!make_pipe(child_input, this->impl->input, true) ||
            !make_pipe(this->impl->output, child_output) ||
            !make_pipe(this->impl->error, child_error)) {
            return failure("Create shell pipes");
        }
        if (fcntl(this->impl->output.get(), F_SETFL, O_NONBLOCK) == -1 ||
            fcntl(this->impl->error.get(), F_SETFL, O_NONBLOCK) == -1) {
            return failure("Set pipe mode");
        }
        this->impl->pid = fork();
        if (this->impl->pid < 0) {
            return failure("Fork shell");
        }
        if (this->impl->pid == 0) {
            constexpr int EXEC_FAILURE = 127;
            std::signal(SIGPIPE, SIG_DFL);
            if (setpgid(0, 0) != 0 || dup2(child_input.get(), STDIN_FILENO) == -1 ||
                dup2(child_output.get(), STDOUT_FILENO) == -1 ||
                dup2(child_error.get(), STDERR_FILENO) == -1) {
                _exit(EXEC_FAILURE);
            }
            execl("/bin/sh", "sh", static_cast<char*>(nullptr));
            _exit(EXEC_FAILURE);
        }
        if (setpgid(this->impl->pid, this->impl->pid) != 0 && errno != EACCES && errno != ESRCH) {
            kill(this->impl->pid, SIGKILL);
            return failure("Create shell process group");
        }
        return {};
    }
    platform::ProcessResult<void> LinuxShellProcess::write(std::string_view data) {
        std::size_t offset{};
        while (offset < data.size()) {
            const auto count = ::send(
                this->impl->input.get(), data.data() + offset, data.size() - offset, MSG_NOSIGNAL);
            if (count < 0 && errno == EINTR) {
                continue;
            }
            if (!(count > 0)) {
                return failure("Write shell input");
            }
            offset += static_cast<std::size_t>(count);
        }
        return {};
    }
    void LinuxShellProcess::close_input() {
        this->impl->input.reset();
    }
    void LinuxShellProcess::terminate() {
        if (this->impl->pid > 0) {
            kill(-this->impl->pid, SIGKILL);
        }
    }
    platform::ProcessResult<std::string> LinuxShellProcess::read(platform::ProcessOutput stream) {
        if (stream != platform::ProcessOutput::Stdout &&
            stream != platform::ProcessOutput::Stderr) {
            return std::unexpected("Invalid shell output stream");
        }
        const auto handle = stream == platform::ProcessOutput::Stdout ? this->impl->output.get()
                                                                      : this->impl->error.get();
        std::array<char, IO_CHUNK_SIZE> bytes{};
        const auto count = ::read(handle, bytes.data(), bytes.size());
        if (count < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                return std::string{};
            }
            return failure("Read shell output");
        }
        return std::string(bytes.data(), static_cast<std::size_t>(count));
    }
    platform::ProcessResult<std::optional<int>> LinuxShellProcess::exit_code() {
        if (this->impl->status.has_value()) {
            return this->impl->status;
        }
        int status{};
        const auto pid = waitpid(this->impl->pid, &status, WNOHANG);
        if (pid == 0 || (pid < 0 && errno == EINTR)) {
            return std::optional<int>{};
        }
        if (pid < 0) {
            return failure("Wait for shell");
        }
        constexpr int SIGNAL_EXIT_BASE = 128;
        this->impl->status =
            WIFEXITED(status) ? WEXITSTATUS(status) : SIGNAL_EXIT_BASE + WTERMSIG(status);
        return this->impl->status;
    }
} // namespace enishi::platform_impl
