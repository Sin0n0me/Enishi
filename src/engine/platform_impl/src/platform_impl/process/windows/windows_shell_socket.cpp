#include "windows_shell_socket.h"
namespace enishi::platform_impl::native_socket {
    Socket create_socket() {
        return ::socket(AF_INET, STREAM_SOCKET, IPPROTO_TCP);
    }
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
    bool initialize() {
        WSADATA data{};
        constexpr BYTE WINSOCK_MAJOR = 2;
        constexpr BYTE WINSOCK_MINOR = 2;
        return WSAStartup(MAKEWORD(WINSOCK_MAJOR, WINSOCK_MINOR), &data) == 0;
    }
    void cleanup() {
        WSACleanup();
    }
    bool set_send_timeout(Socket socket, int milliseconds) {
        const DWORD timeout = static_cast<DWORD>(milliseconds);
        return setsockopt(socket,
                   SOL_SOCKET,
                   SO_SNDTIMEO,
                   reinterpret_cast<const char*>(&timeout),
                   sizeof(timeout)) == 0;
    }
    bool configure_listener(Socket socket) {
        const int exclusive = 1;
        return setsockopt(socket,
                   SOL_SOCKET,
                   SO_EXCLUSIVEADDRUSE,
                   reinterpret_cast<const char*>(&exclusive),
                   sizeof(exclusive)) == 0;
    }
    Socket accept_socket(Socket socket) {
        return ::accept(socket, nullptr, nullptr);
    }
} // namespace enishi::platform_impl::native_socket
