#pragma once
#include <expected>
#include <memory>
#include <optional>
#include <string>

namespace enishi::platform_impl {
    class LinuxTerminalInput {
      private:
        struct Impl;
        std::unique_ptr<Impl> impl;

      public:
        LinuxTerminalInput();
        ~LinuxTerminalInput();
        // nullopt: no input yet; empty string: EOF.
        std::expected<std::optional<std::string>, std::string> read();
    };
    using NativeTerminalInput = LinuxTerminalInput;
} // namespace enishi::platform_impl
