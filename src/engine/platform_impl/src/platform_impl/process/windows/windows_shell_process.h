#pragma once
#include <memory>
#include <optional>
#include <platform/process/interface_shell_process.h>

namespace enishi::platform_impl {
    class WindowsShellProcess final : public platform::IShellProcess {
      private:
        struct Impl;
        std::unique_ptr<Impl> impl;

      public:
        WindowsShellProcess();
        ~WindowsShellProcess() override;
        WindowsShellProcess(const WindowsShellProcess&) = delete;
        WindowsShellProcess& operator=(const WindowsShellProcess&) = delete;
        platform::ProcessResult<void> start() override;
        platform::ProcessResult<void> write(std::string_view data) override;
        void close_input() override;
        void terminate() override;
        // Stdout and Stderr each have one reader; empty means no bytes currently available.
        platform::ProcessResult<std::string> read(platform::ProcessOutput stream) override;
        platform::ProcessResult<std::optional<int>> exit_code() override;
    };
    using NativeShellProcess = WindowsShellProcess;
} // namespace enishi::platform_impl
