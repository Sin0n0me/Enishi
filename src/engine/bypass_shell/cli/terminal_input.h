#pragma once
#include <bypass_shell/shell_connection.h>
#include <optional>

namespace enishi::bypass_shell {
    class TerminalInput {
      private:
        struct Impl;
        std::unique_ptr<Impl> impl;

      public:
        TerminalInput();
        ~TerminalInput();
        // nullopt: no input yet; empty string: EOF.
        ShellResult<std::optional<std::string>> read();
    };
} // namespace enishi::bypass_shell
