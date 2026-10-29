#pragma once
#include <cstdint>
#include <expected>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace enishi::platform_impl {
    template <typename T> using SocketResult = std::expected<T, std::string>;
    class ShellSocket {
      private:
        struct Impl;
        std::unique_ptr<Impl> impl;

      public:
        ShellSocket();
        ~ShellSocket();
        ShellSocket(const ShellSocket&) = delete;
        ShellSocket& operator=(const ShellSocket&) = delete;
        SocketResult<void> connect(std::uint16_t port);
        SocketResult<void> listen(std::uint16_t port);
        SocketResult<std::unique_ptr<ShellSocket>> accept();
        SocketResult<void> send(std::string_view data);
        // Zero means no bytes available; peer closure is an error.
        SocketResult<std::size_t> receive(std::span<char> buffer);
    };
} // namespace enishi::platform_impl
