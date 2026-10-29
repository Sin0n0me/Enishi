#include "shell_socket.h"
#include <utility>
#if defined(_WIN32)
#include "windows/windows_shell_socket.h"
#elif defined(__APPLE__)
#include "macos/macos_shell_socket.h"
#elif defined(__linux__)
#include "linux/linux_shell_socket.h"
#else
#error Unsupported shell platform
#endif

namespace enishi::platform_impl {
    namespace {
        constexpr int SEND_TIMEOUT_MS = 2000;
        constexpr int CONNECTION_BACKLOG = 8;
        std::unexpected<std::string> failure(std::string_view action) {
            return std::unexpected(std::string(action) + " (socket error " +
                                   std::to_string(native_socket::socket_error()) + ")");
        }
        sockaddr_in address(std::uint16_t port) {
            sockaddr_in value{};
            value.sin_family = AF_INET;
            value.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            value.sin_port = htons(port);
            return value;
        }
    } // namespace
    struct ShellSocket::Impl {
        native_socket::Socket socket = native_socket::INVALID_SOCKET_VALUE;
        bool initialized = true;
        inline Impl()
            : initialized(native_socket::initialize()) {
        }
        inline ~Impl() {
            if (this->socket != native_socket::INVALID_SOCKET_VALUE) {
                native_socket::close_socket(this->socket);
            }
            if (this->initialized) {
                native_socket::cleanup();
            }
        }
    };
    ShellSocket::ShellSocket()
        : impl(std::make_unique<Impl>()) {
    }
    ShellSocket::~ShellSocket() = default;

    SocketResult<void> ShellSocket::connect(std::uint16_t port) {
        this->impl = std::make_unique<Impl>();
        if (!this->impl->initialized) {
            return failure("Initialize sockets");
        }
        this->impl->socket = native_socket::create_socket();
        if (this->impl->socket == native_socket::INVALID_SOCKET_VALUE) {
            return failure("Create socket");
        }
        const auto target = address(port);
        if (::connect(this->impl->socket,
                reinterpret_cast<const sockaddr*>(&target),
                sizeof(target)) != 0) {
            return failure("Connect to Enishi; start the application first");
        }
        if (!native_socket::set_send_timeout(this->impl->socket, SEND_TIMEOUT_MS)) {
            return failure("Set send timeout");
        }
        return {};
    }
    SocketResult<void> ShellSocket::listen(std::uint16_t port) {
        this->impl = std::make_unique<Impl>();
        if (!this->impl->initialized) {
            return failure("Initialize sockets");
        }
        this->impl->socket = native_socket::create_socket();
        if (this->impl->socket == native_socket::INVALID_SOCKET_VALUE) {
            return failure("Create listener");
        }
        if (!native_socket::configure_listener(this->impl->socket)) {
            return failure("Configure listener address");
        }
        const auto local = address(port);
        if (::bind(this->impl->socket, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) !=
                0 ||
            ::listen(this->impl->socket, CONNECTION_BACKLOG) != 0 ||
            !native_socket::nonblocking(this->impl->socket)) {
            return failure("Listen on loopback port " + std::to_string(port));
        }
        return {};
    }
    SocketResult<std::unique_ptr<ShellSocket>> ShellSocket::accept() {
        const auto client = native_socket::accept_socket(this->impl->socket);
        if (client == native_socket::INVALID_SOCKET_VALUE) {
            if (native_socket::would_block(native_socket::socket_error())) {
                return nullptr;
            }
            return failure("Accept shell");
        }
        auto connection = std::make_unique<ShellSocket>();
        connection->impl->socket = client;
        if (!native_socket::nonblocking(client)) {
            return failure("Set native_socket::nonblocking connection");
        }
        return connection;
    }
    SocketResult<void> ShellSocket::send(std::string_view data) {
        std::size_t sent{};
        while (sent < data.size()) {
            const auto count = ::send(this->impl->socket,
                data.data() + sent,
                static_cast<int>(data.size() - sent),
                native_socket::SEND_FLAGS);
            if (!(count > 0)) {
                return failure("Send shell event");
            }
            sent += static_cast<std::size_t>(count);
        }
        return {};
    }
    SocketResult<std::size_t> ShellSocket::receive(std::span<char> buffer) {
        const auto count =
            ::recv(this->impl->socket, buffer.data(), static_cast<int>(buffer.size()), 0);
        if (count == 0) {
            return std::unexpected("Shell connection closed");
        }
        if (count < 0) {
            if (native_socket::would_block(native_socket::socket_error())) {
                return std::size_t{};
            }
            return failure("Receive shell event");
        }
        return static_cast<std::size_t>(count);
    }
} // namespace enishi::platform_impl
