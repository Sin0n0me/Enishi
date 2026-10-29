#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>

namespace enishi::platform_impl::native_socket {
    using Socket = SOCKET;
    constexpr Socket INVALID_SOCKET_VALUE = INVALID_SOCKET;
    constexpr int STREAM_SOCKET = SOCK_STREAM;
    inline int socket_error() {
        return WSAGetLastError();
    }
    inline bool would_block(int error) {
        return error == WSAEWOULDBLOCK;
    }
    inline void close_socket(Socket value) {
        closesocket(value);
    }
    inline bool nonblocking(Socket value) {
        u_long enabled = 1;
        return ioctlsocket(value, FIONBIO, &enabled) == 0;
    }
    constexpr int SEND_FLAGS = 0;
    inline bool initialize() {
        WSADATA data{};
        constexpr BYTE WINSOCK_MAJOR = 2;
        constexpr BYTE WINSOCK_MINOR = 2;
        return WSAStartup(MAKEWORD(WINSOCK_MAJOR, WINSOCK_MINOR), &data) == 0;
    }
    inline void cleanup() {
        WSACleanup();
    }
    inline bool set_send_timeout(Socket socket, int milliseconds) {
        const DWORD timeout = static_cast<DWORD>(milliseconds);
        return setsockopt(socket,
                   SOL_SOCKET,
                   SO_SNDTIMEO,
                   reinterpret_cast<const char*>(&timeout),
                   sizeof(timeout)) == 0;
    }
    inline bool configure_listener(Socket socket) {
        const int exclusive = 1;
        return setsockopt(socket,
                   SOL_SOCKET,
                   SO_EXCLUSIVEADDRUSE,
                   reinterpret_cast<const char*>(&exclusive),
                   sizeof(exclusive)) == 0;
    }
    inline Socket accept_socket(Socket socket) {
        return ::accept(socket, nullptr, nullptr);
    }
} // namespace enishi::platform_impl::native_socket
