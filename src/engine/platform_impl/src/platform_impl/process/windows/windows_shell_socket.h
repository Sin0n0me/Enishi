#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>

namespace enishi::platform_impl::native_socket {
    using Socket = SOCKET;
    constexpr Socket INVALID_SOCKET_VALUE = INVALID_SOCKET;
    constexpr int STREAM_SOCKET = SOCK_STREAM;
    Socket create_socket();
    int socket_error();
    bool would_block(int error);
    void close_socket(Socket value);
    bool nonblocking(Socket value);
    constexpr int SEND_FLAGS = 0;
    bool initialize();
    void cleanup();
    bool set_send_timeout(Socket socket, int milliseconds);
    bool configure_listener(Socket socket);
    Socket accept_socket(Socket socket);
} // namespace enishi::platform_impl::native_socket
