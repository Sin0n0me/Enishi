#pragma once
#include "shell_connection.h"
#include <optional>
#include <platform/process/interface_shell_process.h>

namespace enishi::bypass_shell {
    class ShellProcess {
      private:
        std::unique_ptr<platform::IShellProcess> process;

      public:
        explicit ShellProcess(std::unique_ptr<platform::IShellProcess> process);
        ~ShellProcess();
        ShellProcess(const ShellProcess&) = delete;
        ShellProcess& operator=(const ShellProcess&) = delete;
        ShellResult<void> start();
        ShellResult<void> write(std::string_view data);
        void close_input();
        void terminate();
        // Stdout and Stderr each have one reader; empty means no bytes currently available.
        ShellResult<std::string> read(ShellEventKind stream);
        ShellResult<std::optional<int>> exit_code();
    };
} // namespace enishi::bypass_shell
