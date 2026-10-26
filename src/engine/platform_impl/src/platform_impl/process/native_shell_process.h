#pragma once
#include <memory>
#include <optional>
#include <platform/process/interface_shell_process.h>

namespace enishi::platform_impl {
    class NativeShellProcess final : public platform::IShellProcess {
      private:
        struct Impl;
        std::unique_ptr<Impl> impl;

      public:
        NativeShellProcess();
        ~NativeShellProcess() override;
        NativeShellProcess(const NativeShellProcess&) = delete;
        NativeShellProcess& operator=(const NativeShellProcess&) = delete;
        platform::ProcessResult<void> start() override;
        platform::ProcessResult<void> write(std::string_view data) override;
        void close_input() override;
        void terminate() override;
        // Stdout and Stderr each have one reader; empty means no bytes currently available.
        platform::ProcessResult<std::string> read(platform::ProcessOutput stream) override;
        platform::ProcessResult<std::optional<int>> exit_code() override;
    };
    [[nodiscard]] std::unique_ptr<platform::IShellProcess> create_shell_process();
} // namespace enishi::platform_impl
