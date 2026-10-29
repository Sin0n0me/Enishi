#include "windows_shell_process.h"
#include <array>
#include <cstdlib>
#include <span>
#include <utility>
#include <vector>
#define NOMINMAX
#include <windows.h>

namespace enishi::platform_impl {
    namespace {
        constexpr std::size_t IO_CHUNK_SIZE = 4096;
        using Native = HANDLE;
        const Native INVALID_NATIVE = nullptr;
        void release(Native value) {
            CloseHandle(value);
        }
        unsigned native_error() {
            return GetLastError();
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
    } // namespace
    struct WindowsShellProcess::Impl {
        Handle input;
        Handle output;
        Handle error;
        std::optional<int> status;
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
    };
    WindowsShellProcess::WindowsShellProcess()
        : impl(std::make_unique<Impl>()) {
    }
    WindowsShellProcess::~WindowsShellProcess() = default;

    platform::ProcessResult<void> WindowsShellProcess::Impl::launch(
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
    platform::ProcessResult<void> WindowsShellProcess::start() {
        this->impl = std::make_unique<Impl>();
        Handle child_input;
        Handle child_output;
        Handle child_error;
        if (!make_pipe(child_input, this->impl->input, false) ||
            !make_pipe(this->impl->output, child_output, true) ||
            !make_pipe(this->impl->error, child_error, true)) {
            return failure("Create shell pipes");
        }
        return this->impl->launch(child_input, child_output, child_error);
    }
    platform::ProcessResult<void> WindowsShellProcess::write(std::string_view data) {
        std::size_t offset{};
        while (offset < data.size()) {
            DWORD count{};
            if (WriteFile(this->impl->input.get(),
                    data.data() + offset,
                    static_cast<DWORD>(data.size() - offset),
                    &count,
                    nullptr) == FALSE) {
                return failure("Write shell input");
            }
            if (!(count > 0)) {
                return failure("Write shell input");
            }
            offset += static_cast<std::size_t>(count);
        }
        return {};
    }
    void WindowsShellProcess::close_input() {
        this->impl->input.reset();
    }
    void WindowsShellProcess::terminate() {
        if (this->impl->job.get() != nullptr) {
            TerminateJobObject(this->impl->job.get(), EXIT_FAILURE);
        }
    }
    platform::ProcessResult<std::string> WindowsShellProcess::read(platform::ProcessOutput stream) {
        if (stream != platform::ProcessOutput::Stdout &&
            stream != platform::ProcessOutput::Stderr) {
            return std::unexpected("Invalid shell output stream");
        }
        const auto handle = stream == platform::ProcessOutput::Stdout ? this->impl->output.get()
                                                                      : this->impl->error.get();
        std::array<char, IO_CHUNK_SIZE> bytes{};
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
        return std::string(bytes.data(), static_cast<std::size_t>(count));
    }
    platform::ProcessResult<std::optional<int>> WindowsShellProcess::exit_code() {
        if (this->impl->status.has_value()) {
            return this->impl->status;
        }
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
        return this->impl->status;
    }
} // namespace enishi::platform_impl
