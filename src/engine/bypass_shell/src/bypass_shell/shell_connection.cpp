#include "shell_connection.h"
#include <array>
#include <mutex>
#include <platform_impl/process/shell_socket.h>
#include <utility>

namespace enishi::bypass_shell {
    namespace {
        constexpr std::size_t HEADER_SIZE = 9;
        constexpr std::size_t KIND_OFFSET = 4;
        constexpr std::size_t LENGTH_OFFSET = 5;
        constexpr unsigned BYTE_BITS = 8;
        constexpr std::size_t BYTE_MASK = 0xff;
        constexpr std::size_t LENGTH_BYTES = 4;
        constexpr std::string_view MAGIC = "ESH1";
    } // namespace
    struct ShellConnection::Impl {
        std::unique_ptr<platform_impl::ShellSocket> socket =
            std::make_unique<platform_impl::ShellSocket>();
        std::string pending;
        std::mutex send_mutex;
    };
    ShellConnection::ShellConnection()
        : impl(std::make_unique<Impl>()) {
    }
    ShellConnection::~ShellConnection() = default;
    ShellConnection::ShellConnection(ShellConnection&&) noexcept = default;
    ShellConnection& ShellConnection::operator=(ShellConnection&&) noexcept = default;

    ShellResult<void> ShellConnection::connect(std::uint16_t port) {
        this->impl = std::make_unique<Impl>();
        return this->impl->socket->connect(port);
    }
    ShellResult<void> ShellConnection::listen(std::uint16_t port) {
        this->impl = std::make_unique<Impl>();
        return this->impl->socket->listen(port);
    }
    ShellResult<std::unique_ptr<ShellConnection>> ShellConnection::accept() {
        auto socket = this->impl->socket->accept();
        if (!socket.has_value()) {
            return std::unexpected(std::move(socket.error()));
        }
        if (*socket == nullptr) {
            return nullptr;
        }
        auto connection = std::make_unique<ShellConnection>();
        connection->impl->socket = std::move(*socket);
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
        return this->impl->socket->send(frame);
    }
    ShellResult<std::vector<ShellEvent>> ShellConnection::receive() {
        std::array<char, MAX_PAYLOAD> buffer{};
        const auto count = this->impl->socket->receive(buffer);
        if (!count.has_value()) {
            return std::unexpected(count.error());
        }
        this->impl->pending.append(buffer.data(), *count);
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
