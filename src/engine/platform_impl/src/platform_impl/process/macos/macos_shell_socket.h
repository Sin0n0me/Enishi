#pragma once
#include <netinet/in.h>
#include <sys/socket.h>
namespace enishi::platform_impl::native_socket {
    using Socket = int;
    constexpr Socket INVALID_SOCKET_VALUE = -1;
    constexpr int SEND_FLAGS = 0;
    Socket create_socket();
    int socket_error();
    bool would_block(int error);
    void close_socket(Socket value);
    bool nonblocking(Socket value);
    bool initialize();
    void cleanup();
    bool set_send_timeout(Socket socket, int milliseconds);
    bool configure_listener(Socket socket);
    Socket accept_socket(Socket socket);
} // namespace enishi::platform_impl::native_socket
