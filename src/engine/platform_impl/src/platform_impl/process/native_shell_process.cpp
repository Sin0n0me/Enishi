#include "native_shell_process.h"
#include <array>
#include <cstdlib>
#include <span>
#include <utility>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace enishi::platform_impl {
    namespace {
        constexpr std::size_t IO_CHUNK_SIZE = 4096;
#ifdef _WIN32
        using Native = HANDLE;
        const Native INVALID_NATIVE = nullptr;
        void release(Native value) {
            CloseHandle(value);
        }
        unsigned native_error() {
            return GetLastError();
        }
#else
        using Native = int;
        constexpr Native INVALID_NATIVE = -1;
        void release(Native value) {
            ::close(value);
        }
        int native_error() {
            return errno;
        }
#endif
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
#ifdef _WIN32
        bool make_pipe(Handle& read, Handle& write, bool parent_reads) {
            SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
            HANDLE input = nullptr;
            HANDLE output = nullptr;
            if (CreatePipe(&input, &output, &security, 0) == FALSE) {
                return false;
            }
            read.reset(input);
            write.reset(output);
            return SetHandleInformation(parent_reads ? input : output, HANDLE_FLAG_INHERIT, 0) !=
                   FALSE;
        }
        struct Attributes {
            std::vector<std::byte> storage;
            LPPROC_THREAD_ATTRIBUTE_LIST list = nullptr;
            inline ~Attributes() {
                if (this->list != nullptr) {
                    DeleteProcThreadAttributeList(this->list);
                }
            }
            inline bool init(std::span<HANDLE> handles) {
                SIZE_T size{};
                InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
                this->storage.resize(size);
                auto* value = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(this->storage.data());
                if (InitializeProcThreadAttributeList(value, 1, 0, &size) == FALSE) {
                    return false;
                }
                this->list = value;
                return UpdateProcThreadAttribute(value,
                           0,
                           PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                           handles.data(),
                           handles.size_bytes(),
                           nullptr,
                           nullptr) != FALSE;
            }
        };
#else
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
#endif
    } // namespace
    struct NativeShellProcess::Impl {
        Handle input;
        Handle output;
        Handle error;
        std::optional<int> status;
#ifdef _WIN32
        Handle process;
        Handle job;
        platform::ProcessResult<void> launch(
            Handle& child_input, Handle& child_output, Handle& child_error);
        inline ~Impl() {
            this->input.reset();
            this->job.reset(); // KILL_ON_JOB_CLOSE also closes descendants' pipe writers.
            if (this->process.get() != nullptr) {
                WaitForSingleObject(this->process.get(), INFINITE);
            }
        }
#else
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
#endif
    };
    NativeShellProcess::NativeShellProcess()
        : impl(std::make_unique<Impl>()) {
    }
    NativeShellProcess::~NativeShellProcess() = default;

#ifdef _WIN32
    platform::ProcessResult<void> NativeShellProcess::Impl::launch(
        Handle& child_input, Handle& child_output, Handle& child_error) {
        this->job.reset(CreateJobObjectW(nullptr, nullptr));
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (this->job.get() == nullptr ||
            SetInformationJobObject(
                this->job.get(), JobObjectExtendedLimitInformation, &limits, sizeof(limits)) ==
                FALSE) {
            return failure("Create shell job");
        }
        std::array<HANDLE, 3> handles{child_input.get(), child_output.get(), child_error.get()};
        Attributes attributes;
        if (!attributes.init(handles)) {
            return failure("Restrict inherited handles");
        }
        STARTUPINFOEXW startup{};
        startup.StartupInfo.cb = sizeof(startup);
        startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
        startup.StartupInfo.hStdInput = child_input.get();
        startup.StartupInfo.hStdOutput = child_output.get();
        startup.StartupInfo.hStdError = child_error.get();
        startup.lpAttributeList = attributes.list;
        std::array<wchar_t, MAX_PATH> directory{};
        const auto size =
            GetSystemDirectoryW(directory.data(), static_cast<UINT>(directory.size()));
        if (size == 0 || !(size < directory.size())) {
            return failure("Locate cmd.exe");
        }
        const auto executable = std::wstring(directory.data(), size) + L"\\cmd.exe";
        std::wstring command = L"cmd.exe /D /Q";
        PROCESS_INFORMATION process_info{};
        DWORD flags = CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT | CREATE_NEW_PROCESS_GROUP;
        // Reuse an existing console so cmd and the relay agree on the active code page.
        if (GetConsoleCP() == 0) {
            flags |= CREATE_NO_WINDOW;
        }
        if (CreateProcessW(executable.c_str(),
                command.data(),
                nullptr,
                nullptr,
                TRUE,
                flags,
                nullptr,
                nullptr,
                &startup.StartupInfo,
                &process_info) == FALSE) {
            return failure("Start cmd.exe");
        }
        this->process.reset(process_info.hProcess);
        Handle thread;
        thread.reset(process_info.hThread);
        if (AssignProcessToJobObject(this->job.get(), process_info.hProcess) == FALSE ||
            ResumeThread(thread.get()) == static_cast<DWORD>(-1)) {
            TerminateProcess(process_info.hProcess, EXIT_FAILURE);
            return failure("Start shell job");
        }
        return {};
    }
#endif
    platform::ProcessResult<void> NativeShellProcess::start() {
        this->impl = std::make_unique<Impl>();
        Handle child_input;
        Handle child_output;
        Handle child_error;
#ifdef _WIN32
        if (!make_pipe(child_input, this->impl->input, false) ||
            !make_pipe(this->impl->output, child_output, true) ||
            !make_pipe(this->impl->error, child_error, true)) {
            return failure("Create shell pipes");
        }
        return this->impl->launch(child_input, child_output, child_error);
#else
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
#endif
    }
    platform::ProcessResult<void> NativeShellProcess::write(std::string_view data) {
        std::size_t offset{};
        while (offset < data.size()) {
#ifdef _WIN32
            DWORD count{};
            if (WriteFile(this->impl->input.get(),
                    data.data() + offset,
                    static_cast<DWORD>(data.size() - offset),
                    &count,
                    nullptr) == FALSE) {
                return failure("Write shell input");
            }
#else
            const auto count = ::send(
                this->impl->input.get(), data.data() + offset, data.size() - offset, MSG_NOSIGNAL);
            if (count < 0 && errno == EINTR) {
                continue;
            }
#endif
            if (!(count > 0)) {
                return failure("Write shell input");
            }
            offset += static_cast<std::size_t>(count);
        }
        return {};
    }
    void NativeShellProcess::close_input() {
        this->impl->input.reset();
    }
    void NativeShellProcess::terminate() {
#ifdef _WIN32
        if (this->impl->job.get() != nullptr) {
            TerminateJobObject(this->impl->job.get(), EXIT_FAILURE);
        }
#else
        if (this->impl->pid > 0) {
            kill(-this->impl->pid, SIGKILL);
        }
#endif
    }
    platform::ProcessResult<std::string> NativeShellProcess::read(platform::ProcessOutput stream) {
        if (stream != platform::ProcessOutput::Stdout &&
            stream != platform::ProcessOutput::Stderr) {
            return std::unexpected("Invalid shell output stream");
        }
        const auto handle = stream == platform::ProcessOutput::Stdout ? this->impl->output.get()
                                                                      : this->impl->error.get();
        std::array<char, IO_CHUNK_SIZE> bytes{};
#ifdef _WIN32
        DWORD available{};
        if (PeekNamedPipe(handle, nullptr, 0, nullptr, &available, nullptr) == FALSE) {
            if (GetLastError() == ERROR_BROKEN_PIPE) {
                return std::string{};
            }
            return failure("Poll shell output");
        }
        if (available == 0) {
            return std::string{};
        }
        DWORD count{};
        if (ReadFile(handle, bytes.data(), static_cast<DWORD>(bytes.size()), &count, nullptr) ==
            FALSE) {
            return failure("Read shell output");
        }
#else
        const auto count = ::read(handle, bytes.data(), bytes.size());
        if (count < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                return std::string{};
            }
            return failure("Read shell output");
        }
#endif
        return std::string(bytes.data(), static_cast<std::size_t>(count));
    }
    platform::ProcessResult<std::optional<int>> NativeShellProcess::exit_code() {
        if (this->impl->status.has_value()) {
            return this->impl->status;
        }
#ifdef _WIN32
        const auto wait = WaitForSingleObject(this->impl->process.get(), 0);
        if (wait == WAIT_TIMEOUT) {
            return std::optional<int>{};
        }
        if (wait != WAIT_OBJECT_0) {
            return failure("Wait for shell");
        }
        DWORD code{};
        if (GetExitCodeProcess(this->impl->process.get(), &code) == FALSE) {
            return failure("Get shell exit code");
        }
        this->impl->status = static_cast<int>(code);
#else
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
#endif
        return this->impl->status;
    }
    std::unique_ptr<platform::IShellProcess> create_shell_process() {
        return std::make_unique<NativeShellProcess>();
    }
} // namespace enishi::platform_impl
