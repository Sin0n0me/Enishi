#include "shell_connection.h"
#include <array>
#include <mutex>
#include <utility>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace enishi::bypass_shell {
    namespace {
        constexpr std::size_t HEADER_SIZE = 9;
        constexpr std::size_t KIND_OFFSET = 4;
        constexpr std::size_t LENGTH_OFFSET = 5;
        constexpr unsigned BYTE_BITS = 8;
        constexpr std::size_t BYTE_MASK = 0xff;
        constexpr std::size_t LENGTH_BYTES = 4;
        constexpr int SEND_TIMEOUT_MS = 2000;
        constexpr int CONNECTION_BACKLOG = 8;
        constexpr std::string_view MAGIC = "ESH1";
#ifdef _WIN32
        using Socket = SOCKET;
        constexpr Socket INVALID_SOCKET_VALUE = INVALID_SOCKET;
        constexpr int STREAM_SOCKET = SOCK_STREAM;
        int socket_error() {
            return WSAGetLastError();
        }
        bool would_block(int error) {
            return error == WSAEWOULDBLOCK;
        }
        void close_socket(Socket value) {
            closesocket(value);
        }
        bool nonblocking(Socket value) {
            u_long enabled = 1;
            return ioctlsocket(value, FIONBIO, &enabled) == 0;
        }
#else
        using Socket = int;
        constexpr Socket INVALID_SOCKET_VALUE = -1;
        constexpr int STREAM_SOCKET = SOCK_STREAM | SOCK_CLOEXEC;
        int socket_error() {
            return errno;
        }
        bool would_block(int error) {
            return error == EAGAIN || error == EWOULDBLOCK;
        }
        void close_socket(Socket value) {
            ::close(value);
        }
        bool nonblocking(Socket value) {
            const auto flags = fcntl(value, F_GETFL);
            return flags != -1 && fcntl(value, F_SETFL, flags | O_NONBLOCK) != -1;
        }
#endif
        std::unexpected<std::string> failure(std::string_view action) {
            return std::unexpected(
                std::string(action) + " (socket error " + std::to_string(socket_error()) + ")");
        }
        sockaddr_in address(std::uint16_t port) {
            sockaddr_in value{};
            value.sin_family = AF_INET;
            value.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            value.sin_port = htons(port);
            return value;
        }
    } // namespace
    struct ShellConnection::Impl {
        Socket socket = INVALID_SOCKET_VALUE;
        std::string pending;
        std::mutex send_mutex;
        bool initialized = true;
        inline Impl() {
#ifdef _WIN32
            WSADATA data{};
            this->initialized = WSAStartup(MAKEWORD(2, 2), &data) == 0;
#endif
        }
        inline ~Impl() {
            if (this->socket != INVALID_SOCKET_VALUE) {
                close_socket(this->socket);
            }
#ifdef _WIN32
            if (this->initialized) {
                WSACleanup();
            }
#endif
        }
    };
    ShellConnection::ShellConnection()
        : impl(std::make_unique<Impl>()) {
    }
    ShellConnection::~ShellConnection() = default;
    ShellConnection::ShellConnection(ShellConnection&&) noexcept = default;
    ShellConnection& ShellConnection::operator=(ShellConnection&&) noexcept = default;

    ShellResult<void> ShellConnection::connect(std::uint16_t port) {
        this->impl = std::make_unique<Impl>();
        if (!this->impl->initialized) {
            return failure("Initialize sockets");
        }
        this->impl->socket = ::socket(AF_INET, STREAM_SOCKET, IPPROTO_TCP);
        if (this->impl->socket == INVALID_SOCKET_VALUE) {
            return failure("Create socket");
        }
        const auto target = address(port);
        if (::connect(this->impl->socket,
                reinterpret_cast<const sockaddr*>(&target),
                sizeof(target)) != 0) {
            return failure("Connect to Enishi; start the application first");
        }
#ifdef _WIN32
        const DWORD timeout = SEND_TIMEOUT_MS;
#else
        constexpr int MILLISECONDS_PER_SECOND = 1000;
        const timeval timeout{SEND_TIMEOUT_MS / MILLISECONDS_PER_SECOND, 0};
#endif
        if (setsockopt(this->impl->socket,
                SOL_SOCKET,
                SO_SNDTIMEO,
                reinterpret_cast<const char*>(&timeout),
                sizeof(timeout)) != 0) {
            return failure("Set send timeout");
        }
        return {};
    }
    ShellResult<void> ShellConnection::listen(std::uint16_t port) {
        this->impl = std::make_unique<Impl>();
        if (!this->impl->initialized) {
            return failure("Initialize sockets");
        }
        this->impl->socket = ::socket(AF_INET, STREAM_SOCKET, IPPROTO_TCP);
        if (this->impl->socket == INVALID_SOCKET_VALUE) {
            return failure("Create listener");
        }
#ifdef _WIN32
        const int exclusive = 1;
        if (setsockopt(this->impl->socket,
                SOL_SOCKET,
                SO_EXCLUSIVEADDRUSE,
                reinterpret_cast<const char*>(&exclusive),
                sizeof(exclusive)) != 0) {
            return failure("Set exclusive listener");
        }
#else
        const int reuse = 1;
        if (setsockopt(this->impl->socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0) {
            return failure("Set listener address reuse");
        }
#endif
        const auto local = address(port);
        if (::bind(this->impl->socket, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) !=
                0 ||
            ::listen(this->impl->socket, CONNECTION_BACKLOG) != 0 ||
            !nonblocking(this->impl->socket)) {
            return failure("Listen on loopback port " + std::to_string(port));
        }
        return {};
    }
    ShellResult<std::unique_ptr<ShellConnection>> ShellConnection::accept() {
#ifdef _WIN32
        const auto client = ::accept(this->impl->socket, nullptr, nullptr);
#else
        const auto client = ::accept4(this->impl->socket, nullptr, nullptr, SOCK_CLOEXEC);
#endif
        if (client == INVALID_SOCKET_VALUE) {
            if (would_block(socket_error())) {
                return nullptr;
            }
            return failure("Accept shell");
        }
        auto connection = std::make_unique<ShellConnection>();
        connection->impl->socket = client;
        if (!nonblocking(client)) {
            return failure("Set nonblocking connection");
        }
        return connection;
    }
    ShellResult<void> ShellConnection::send(ShellEventKind kind, std::string_view data) {
        if (data.size() > MAX_PAYLOAD) {
            return std::unexpected("Shell event exceeds payload limit");
        }
        std::lock_guard lock(this->impl->send_mutex);
        std::string frame(MAGIC);
        frame.push_back(static_cast<char>(kind));
        // Network length = sum(byte[i] * 2^(8 * (3-i))).
        for (std::size_t index = 0; index < LENGTH_BYTES; ++index) {
            frame.push_back(static_cast<char>(
                (data.size() >> (BYTE_BITS * (LENGTH_BYTES - index - 1))) & BYTE_MASK));
        }
        frame.append(data);
        std::size_t sent{};
        while (sent < frame.size()) {
#ifdef _WIN32
            constexpr int flags = 0;
#else
            constexpr int flags = MSG_NOSIGNAL;
#endif
            const auto count = ::send(this->impl->socket,
                frame.data() + sent,
                static_cast<int>(frame.size() - sent),
                flags);
            if (!(count > 0)) {
                return failure("Send shell event");
            }
            sent += static_cast<std::size_t>(count);
        }
        return {};
    }
    ShellResult<std::vector<ShellEvent>> ShellConnection::receive() {
        std::array<char, MAX_PAYLOAD> buffer{};
        const auto count =
            ::recv(this->impl->socket, buffer.data(), static_cast<int>(buffer.size()), 0);
        if (count == 0) {
            return std::unexpected("Shell connection closed");
        }
        if (count < 0 && !would_block(socket_error())) {
            return failure("Receive shell event");
        }
        if (count > 0) {
            this->impl->pending.append(buffer.data(), static_cast<std::size_t>(count));
        }
        std::vector<ShellEvent> events;
        auto& pending = this->impl->pending;
        while (!(pending.size() < HEADER_SIZE)) {
            if (!pending.starts_with(MAGIC)) {
                return std::unexpected("Invalid shell protocol");
            }
            const auto kind =
                static_cast<ShellEventKind>(static_cast<unsigned char>(pending[KIND_OFFSET]));
            if (kind < ShellEventKind::Started || kind > ShellEventKind::Exited) {
                return std::unexpected("Invalid shell event kind");
            }
            std::size_t size{};
            for (std::size_t index = 0; index < LENGTH_BYTES; ++index) {
                size = (size << BYTE_BITS) |
                       static_cast<unsigned char>(pending[LENGTH_OFFSET + index]);
            }
            if (size > MAX_PAYLOAD) {
                return std::unexpected("Shell payload exceeds limit");
            }
            if (pending.size() < HEADER_SIZE + size) {
                break;
            }
            events.push_back({0, kind, pending.substr(HEADER_SIZE, size)});
            pending.erase(0, HEADER_SIZE + size);
        }
        return events;
    }
} // namespace enishi::bypass_shell
