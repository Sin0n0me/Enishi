#pragma once
#include <expected>
#include <memory>
#include <optional>
#include <string>

namespace enishi::platform_impl {
    class WindowsTerminalInput {
      private:
        struct Impl;
        std::unique_ptr<Impl> impl;

      public:
        WindowsTerminalInput();
        ~WindowsTerminalInput();
        // nullopt: no input yet; empty string: EOF.
        std::expected<std::optional<std::string>, std::string> read();
    };
    using NativeTerminalInput = WindowsTerminalInput;
} // namespace enishi::platform_impl
