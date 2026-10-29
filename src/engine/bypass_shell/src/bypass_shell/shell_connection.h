#pragma once
#include "shell_event.h"
#include <expected>
#include <memory>
#include <string_view>
#include <vector>

namespace enishi::bypass_shell {
    template <typename T> using ShellResult = std::expected<T, std::string>;
    class ShellConnection {
      private:
        struct Impl;
        std::unique_ptr<Impl> impl;

      public:
        ShellConnection();
        ~ShellConnection();
        ShellConnection(ShellConnection&&) noexcept;
        ShellConnection& operator=(ShellConnection&&) noexcept;
        ShellConnection(const ShellConnection&) = delete;
        ShellConnection& operator=(const ShellConnection&) = delete;
        ShellResult<void> connect(std::uint16_t port);
        ShellResult<void> listen(std::uint16_t port);
        ShellResult<std::unique_ptr<ShellConnection>> accept();
        ShellResult<void> send(ShellEventKind kind, std::string_view data);
        // Empty batch means no complete frame is available; EOF is an error.
        ShellResult<std::vector<ShellEvent>> receive();
    };
} // namespace enishi::bypass_shell
