#pragma once
#include <expected>
#include <memory>
#include <optional>
#include <string>

namespace enishi::platform_impl {
    class PosixTerminalInput {
      private:
        struct Impl;
        std::unique_ptr<Impl> impl;

      public:
        PosixTerminalInput();
        ~PosixTerminalInput();
        // nullopt: no input yet; empty string: EOF.
        std::expected<std::optional<std::string>, std::string> read();
    };
    using NativeTerminalInput = PosixTerminalInput;
} // namespace enishi::platform_impl
