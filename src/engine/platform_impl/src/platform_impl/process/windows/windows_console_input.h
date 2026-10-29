#pragma once
#include <expected>
#include <memory>
#include <optional>
#include <string>
namespace enishi::platform_impl {
    [[nodiscard]] bool has_console_terminal();
    class WindowsConsoleInput {
      private:
        struct Impl;
        std::unique_ptr<Impl> impl;

      public:
        WindowsConsoleInput();
        ~WindowsConsoleInput();
        std::expected<std::optional<std::string>, std::string> read();
    };
} // namespace enishi::platform_impl
