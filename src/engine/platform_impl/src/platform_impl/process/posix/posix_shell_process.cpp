#include "posix_shell_process.h"
#include <array>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <poll.h>
#include <span>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>
#if defined(__APPLE__)
#include "../macos/macos_process_io.h"
#else
#include "../linux/linux_process_io.h"
#endif

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
            const auto status = native_process::create_pipe(descriptors, socket_input);
            if (status != 0) {
                return false;
            }
            read.reset(descriptors[0]);
            write.reset(descriptors[1]);
            return native_process::configure_descriptor(read.get(), socket_input) &&
                   native_process::configure_descriptor(write.get(), socket_input);
        }
    } // namespace
    struct PosixShellProcess::Impl {
        Handle input;
        Handle output;
        Handle error;
        std::optional<int> status;
        pid_t pid = -1;
        bool terminal = false;
        winsize size{};
        platform::ProcessResult<void> start_terminal();
        void terminate();
        inline ~Impl() {
            this->terminate();
            this->input.reset();
            if (this->pid > 0) {
                if (!this->status.has_value()) {
                    while (waitpid(this->pid, nullptr, 0) < 0 && errno == EINTR) {
                    }
                }
            }
        }
    };
    void PosixShellProcess::Impl::terminate() {
        if (this->terminal && this->output.get() != INVALID_NATIVE) {
            const auto foreground = tcgetpgrp(this->output.get());
            if (foreground > 0 && foreground != this->pid) {
                kill(-foreground, SIGKILL);
            }
        }
        if (this->pid > 0) {
            kill(-this->pid, SIGKILL);
        }
    }
    platform::ProcessResult<void> PosixShellProcess::Impl::start_terminal() {
        this->terminal = true;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &this->size) == -1) {
            return failure("Read terminal size");
        }
        int master = INVALID_NATIVE;
        int slave = INVALID_NATIVE;
        if (native_process::open_terminal(master, slave, this->size) == -1) {
            return failure("Create pseudo terminal");
        }
        this->output.reset(master);
        Handle child;
        child.reset(slave);
        this->input.reset(fcntl(master, F_DUPFD_CLOEXEC, STDERR_FILENO + 1));
        if (this->input.get() == INVALID_NATIVE || fcntl(master, F_SETFD, FD_CLOEXEC) == -1 ||
            fcntl(slave, F_SETFD, FD_CLOEXEC) == -1 || fcntl(master, F_SETFL, O_NONBLOCK) == -1) {
            return failure("Configure pseudo terminal");
        }
        const auto limit = native_process::descriptor_limit();
        if (limit < 0) {
            return failure("Get descriptor limit");
        }
        this->pid = fork();
        if (this->pid < 0) {
            return failure("Fork terminal shell");
        }
        if (this->pid == 0) {
            constexpr int EXEC_FAILURE = 127;
            std::signal(SIGPIPE, SIG_DFL);
            if (setsid() == -1 || ioctl(slave, TIOCSCTTY, 0) == -1 ||
                dup2(slave, STDIN_FILENO) == -1 || dup2(slave, STDOUT_FILENO) == -1 ||
                dup2(slave, STDERR_FILENO) == -1) {
                _exit(EXEC_FAILURE);
            }
            native_process::prepare_exec(limit);
            execl("/bin/sh", "sh", "-i", static_cast<char*>(nullptr));
            _exit(EXEC_FAILURE);
        }
        return {};
    }
    PosixShellProcess::PosixShellProcess()
        : impl(std::make_unique<Impl>()) {
    }
    PosixShellProcess::~PosixShellProcess() = default;

    platform::ProcessResult<void> PosixShellProcess::start() {
        this->impl = std::make_unique<Impl>();
        if (isatty(STDIN_FILENO) == 1 && isatty(STDOUT_FILENO) == 1) {
            return this->impl->start_terminal();
        }
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
        const auto descriptor_limit = native_process::descriptor_limit();
        if (descriptor_limit < 0) {
            return failure("Get descriptor limit");
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
            native_process::prepare_exec(descriptor_limit);
            execl("/bin/sh", "sh", static_cast<char*>(nullptr));
            _exit(EXEC_FAILURE);
        }
        if (setpgid(this->impl->pid, this->impl->pid) != 0 && errno != EACCES && errno != ESRCH) {
            kill(this->impl->pid, SIGKILL);
            return failure("Create shell process group");
        }
        return {};
    }
    platform::ProcessResult<void> PosixShellProcess::write(std::string_view data) {
        std::size_t offset{};
        while (offset < data.size()) {
            const auto count =
                this->impl->terminal
                    ? ::write(this->impl->input.get(), data.data() + offset, data.size() - offset)
                    : ::send(this->impl->input.get(),
                          data.data() + offset,
                          data.size() - offset,
                          native_process::SEND_FLAGS);
            if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                pollfd writable{this->impl->input.get(), POLLOUT, 0};
                if (poll(&writable, 1, -1) < 0 && errno != EINTR) {
                    return failure("Wait for terminal input");
                }
                continue;
            }
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
    void PosixShellProcess::close_input() {
        if (this->impl->terminal) {
            this->impl->terminate();
        }
        this->impl->input.reset();
    }
    void PosixShellProcess::terminate() {
        this->impl->terminate();
    }
    platform::ProcessResult<std::string> PosixShellProcess::read(platform::ProcessOutput stream) {
        if (stream != platform::ProcessOutput::Stdout &&
            stream != platform::ProcessOutput::Stderr) {
            return std::unexpected("Invalid shell output stream");
        }
        if (this->impl->terminal && stream == platform::ProcessOutput::Stderr) {
            return std::string{};
        }
        const auto handle = stream == platform::ProcessOutput::Stdout ? this->impl->output.get()
                                                                      : this->impl->error.get();
        std::array<char, IO_CHUNK_SIZE> bytes{};
        const auto count = ::read(handle, bytes.data(), bytes.size());
        if (count < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ||
                (this->impl->terminal && errno == EIO)) {
                return std::string{};
            }
            return failure("Read shell output");
        }
        return std::string(bytes.data(), static_cast<std::size_t>(count));
    }
    platform::ProcessResult<std::optional<int>> PosixShellProcess::exit_code() {
        if (this->impl->status.has_value()) {
            return this->impl->status;
        }
        if (this->impl->terminal) {
            winsize size{};
            if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == -1) {
                return failure("Read terminal size");
            }
            if (size.ws_col != this->impl->size.ws_col || size.ws_row != this->impl->size.ws_row) {
                if (ioctl(this->impl->output.get(), TIOCSWINSZ, &size) == -1) {
                    return failure("Resize pseudo terminal");
                }
                this->impl->size = size;
            }
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
